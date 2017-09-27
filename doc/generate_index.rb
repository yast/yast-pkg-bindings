#! /usr/bin/ruby

# @return [Array] list of builtns
def builtins
  files = Dir.glob(File.expand_path("../../src/*.{h,cc}", __FILE__))
  res = []
  files.each do |source_path|
    lines = File.readlines(source_path)
    puts "processing #{source_path}"
    lines.each do |l|
      case l
      when /@builtin/
        puts "found #{l}"
        builtin = l[/@builtin\s+(.*\S)\s*$/, 1]
        res << builtin
      end
    end
  end

  res
end

path = File.expand_path("../../doc/index.md", __FILE__)
File.open(path, "w") do |file|
  file.puts ""
  file.puts "Yast::Pkg methods:"
  file.puts "-------------"
  file.puts ""
  builtins.sort.each do |builtin|
    file.puts "- [#{builtin}](@ref Yast::Pkg::#{builtin})"
  end
end

path = File.expand_path("../../doc/yast.h", __FILE__)
File.open(path, "w") do |file|
  file.puts "namespace Yast {"
  file.puts "/**"
  file.puts "  * Yast module for Pkg bindings"
  file.puts "  */"
  file.puts "  class Pkg {"
  file.puts "  public:"
  builtins.sort.each do |builtin|
    file.puts "void #{builtin}();" # TODO add params
  end
  file.puts "  }"
  file.puts "}"
end
