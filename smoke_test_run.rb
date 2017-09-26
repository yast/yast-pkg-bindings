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

# Check all repositories - this expects at least one repo is defined in the system
puts "Checking Pkg.SourceGetCurrent..."
repos = Yast::Pkg.SourceGetCurrent(false)
raise "Pkg.SourceGetCurrent failed!" unless repos
raise "No repository found!" if repos.empty?
puts "OK (found #{repos.size} repositories)"

# Check all packages - this expects at least one package is available/installed
puts "Checking Pkg.ResolvableProperties..."
packages = Yast::Pkg.ResolvableProperties("", :package, "")
raise "Pkg.ResolvableProperties failed!" unless packages
raise "No package found!" if packages.empty?
puts "OK (found #{packages.size} packages)"

# scan y2log for errors
check_y2log
