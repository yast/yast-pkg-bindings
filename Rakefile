require "yast/rake"

Yast::Tasks.submit_to :sle15sp5

Yast::Tasks.configuration do |conf|
  #lets ignore license check for now
  conf.skip_license_check << /.*/
end

task :doc do
  rm_rf "autodocs"
  mkdir "autodocs"
  sh "doc/generate_index.rb"
  sh "doxygen Doxyfile"
end
