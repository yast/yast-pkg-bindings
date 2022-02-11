#! /usr/bin/env ruby

# the used y2log file
def log_file
  Process.uid.zero? ? "/var/log/YaST2/y2log" : "#{ENV["HOME"]}/.y2log"
end

# ASAP, initializing YaST does some logging!
raise "Non empty #{log_file} log file found!" if File.size?(log_file)

require "yast"
# import the pkg-bindings module
Yast.import "Pkg"

# grep y2log for errors, this is a bit fragile but some YaST errors go only
# to y2log without affecting the return value :-(
def check_y2log
  y2log = File.read(log_file).split("\n")

  # keep only errors and higher
  y2log.select! { |l| l =~ /^[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2} <[3-5]>/ }

  # ignore internal libzypp exceptions when probing the /media.1/media file
  y2log.reject! { |l| l =~ /\/media.1\/media/ }
  # empty exception line following the previous error
  y2log.reject! { |l| l =~ /Exception.cc\(log\):[0-9]+\s*$/ }

  # ignore "Can't openfile '/var/lib/zypp/LastDistributionFlavor' for writing"
  # (when running as non-root)
  y2log.reject! { |l| l =~ /\/var\/lib\/zypp\/LastDistributionFlavor/ }

  # no idea why that happens at Travis, let's ignore that...
  y2log.reject! { |l| l =~ /error: Interrupted system call/ }

  if !y2log.empty?
    puts "Found errors in #{log_file}:"
    puts y2log
    raise "Found errors in y2log!"
  end
end

# Initialize the target, you can use Installation.destdir instead
# of the hardcoded "/" path.
puts "Checking Pkg.TargetInitialize..."
raise "Pkg.TargetInitialize failed!" unless Yast::Pkg.TargetInitialize("/")
puts "OK"

# Load the installed packages into the pool.
puts "Checking Pkg.TargetLoad..."
raise "Pkg.TargetLoad failed!" unless Yast::Pkg.TargetLoad
puts "OK"

# Load the repository configuration. Refreshes the repositories if needed.
puts "Checking Pkg.SourceRestore..."
raise "Pkg.SourceRestore failed!" unless Yast::Pkg.SourceRestore
puts "OK"

# Load the available packages in the repositories to the pool.
puts "Checking Pkg.SourceLoad..."
raise "Pkg.SourceLoad failed!" unless Yast::Pkg.SourceLoad
puts "OK"


puts "Found #{Yast::Pkg.Resolvables({kind: :package}, []).size} packages"
installed_packages = Yast::Pkg.Resolvables({kind: :package, status: :installed}, [])
puts "Found #{installed_packages.size} installed packages"

# Check all repositories - this expects at least one repo is defined in the system
puts "Checking Pkg.SourceGetCurrent..."
repos = Yast::Pkg.SourceGetCurrent(false)
raise "Pkg.SourceGetCurrent failed!" unless repos
raise "No repository found!" if repos.empty?
puts "OK (found #{repos.size} repositories)"

# Check the repository properties
puts "Checking Pkg.SourceGeneralData..."
repo_data = Yast::Pkg.SourceGeneralData(repos.first)
repo_path = repo_data["file"]
raise "Unexpected repository location: #{repo_path.inspect}" unless repo_path&.start_with?("/etc/zypp/repos.d/")
puts "OK (the first repository was read from #{repo_path})"

# Check all packages - this expects at least one package is available/installed
puts "Checking Pkg.ResolvableProperties..."
packages = Yast::Pkg.ResolvableProperties("", :package, "")
raise "Pkg.ResolvableProperties failed!" unless packages
raise "No package found!" if packages.empty?
puts "OK (found #{packages.size} packages)"

# make sure a name without variables stays the same
puts "Checking Pkg.ExpandedName..."
name = "YaST"
expanded_name = Yast::Pkg.ExpandedName(name)
if name != expanded_name
  raise "Unexpected result: #{expanded_name.inspect}, expected #{name.inspect}"
end
puts "OK"

# make sure a name with variables does not stay the same
puts "Checking Pkg.ExpandedName..."
name = "YaST $releasever"
expanded_name = Yast::Pkg.ExpandedName(name)
if name == expanded_name
  raise "Unexpected result: #{expanded_name.inspect} is not expanded"
end
puts "OK"

# make sure no URL part is lost by Pkg.ExpandedUrl call (bsc#1067007)
puts "Checking Pkg.ExpandedUrl..."
url = "https://user:pwd@example.com/path?opt=value"
expanded_url = Yast::Pkg.ExpandedUrl(url)
if url != expanded_url
  raise "Unexpected result: #{expanded_url.inspect}, expected #{url.inspect}"
end
puts "OK"

# Check all packages - this expects at least one package is available/installed
puts "Checking Pkg.Resolvables..."
resolvables = Yast::Pkg.Resolvables({kind: :package}, [])
raise "Pkg.Resolvables failed!" unless resolvables
raise "No package found!" if resolvables.empty?
# compare with the old Pkg.ResolvableProperties call
raise "Different number of packages found!" if packages.size != resolvables.size
puts "OK (found #{resolvables.size} packages)"

patterns = Yast::Pkg.Resolvables({kind: :pattern}, [:name])
raise "Pkg.Resolvables failed!" unless patterns
raise "No pattern found!" if patterns.empty?
puts "OK (found #{patterns.size} patterns)"

installed_packages = Yast::Pkg.Resolvables({kind: :package, status: :installed, name: "yast2-core"}, [:name])
raise "yast2-core package not installed (???)" if installed_packages.empty?
raise "several yast2-core packages installed (???)" if installed_packages.size > 1
puts "OK (yast2-core package installed)"

selected_packages = Yast::Pkg.Resolvables({kind: :package, status: :selected}, [:name])
raise "A package is selected to install (???)" unless selected_packages.empty?
puts "OK (no package selected)"

obsolete_packages = Yast::Pkg.Resolvables({kind: :package, obsoletes_regexp: "^yast2-config-"}, [])
raise "No package with yast2-config obsoletes found" if obsolete_packages.empty?
puts "OK (found #{obsolete_packages.size} packages with yast2-config-* obsoletes)"

supplement_packages = Yast::Pkg.Resolvables({kind: :package, supplements_regexp: "^autoyast\\(.*\\)"}, [])
raise "No package with autoyast() supplements found" if supplement_packages.empty?
puts "OK (found #{supplement_packages.size} packages with autoyast() supplements)"

provide_packages = Yast::Pkg.Resolvables({kind: :package, provides: "y2c_online_update"}, [])
raise "No package with y2c_online_update provides found" if provide_packages.empty?
puts "OK (found #{provide_packages.size} packages providing y2c_online_update)"

removed = Yast::Pkg.ResolvableRemove("yast2-core", :package)
raise "Cannot select yast2-core to uninstall" unless removed
puts "OK (yast2-core selected to uninstall"

selected_packages = Yast::Pkg.Resolvables({kind: :package, status: :selected}, [:name])
raise "A package is selected to install (???)" unless selected_packages.empty?
puts "OK (no package selected)"

removed_packages = Yast::Pkg.Resolvables({kind: :package, status: :removed}, [:name])
raise "No package to uninstall" if removed_packages.empty?
raise "yast2-core not selected to uninstall" unless removed_packages.include?({"name" => "yast2-core"})
puts "OK (yast2-core selected to uninstall)"

installed_products = Yast::Pkg.Resolvables({kind: :product, status: :installed}, [:name, :display_name])
available_products = Yast::Pkg.Resolvables({kind: :product, status: :available}, [:name, :display_name])
selected_products = Yast::Pkg.Resolvables({kind: :product, status: :selected}, [:name, :display_name])
raise "Pkg.Resolvables failed!" unless patterns
raise "No installed product found!" if installed_products.empty?
raise "No available product found!" if available_products.empty?
raise "A selected product found, nothing should be selected now!" unless selected_products.empty?
puts "Found #{installed_products.size} installed products: #{installed_products.map{|p| p["display_name"]}}"
puts "Found #{available_products.size} available products: #{available_products.map{|p| p["display_name"]}}"
puts "OK"

# scan y2log for errors
check_y2log
