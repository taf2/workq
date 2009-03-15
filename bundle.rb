#!/usr/bin/env ruby

class Installer
  attr_reader :app_contents, :app_frameworks, :app_binpath, :app_resources

  def initialize
    @app_contents   = "WorkqViz.app/Contents"
    @app_frameworks = "#{app_contents}/Frameworks"
    @app_resources = "#{app_contents}/Resources"
    @app_macos    = "#{app_contents}/MacOS"
    @app_binpath    = "#{app_contents}/MacOS/workqviz"
    @relocated_list = {}
    @installed_list = {}
  end

  def bindeps(binary)
    otool = `otool -L #{binary}`.strip.split("\n")
    otool.shift
    bindeps = []
    for lib in otool do
      path = lib.sub(/\t/,'').gsub(/\s.*$/,'').strip
      bindeps << path
    end
    bindeps
  end

  def relocate_lib(libbin)
    return puts("relocated #{lib}") if @relocated_list.key?(libbin)
    deps = bindeps(libbin)
    changelibs = nonsystem_libs(deps).reject{|l| l == libbin }

    libbin_name = File.basename(libbin)

    #puts "For #{libbin_name} relocating..."

    for lib in changelibs do
      install_lib(lib) do|lib,basename|
        # update the executable information
        #puts "updating #{lib} for #{libbin_name}"
        cmd = "install_name_tool -change #{lib} @executable_path/../Frameworks/#{basename} #{app_frameworks}/#{libbin_name}"
        #puts cmd
        system(cmd)
      end
    end
    @relocated_list[libbin] = true
  end

  def install_lib(lib)
    basename = File.basename(lib)
    if !@installed_list.key?(lib)
      magic = lib.gsub(/\d.*/,'*.dylib')
      copy = `which cp`.strip
      cmd = "#{copy} --no-dereference --preserve=links #{magic} #{app_frameworks}"
      # copy the lib to the frameworks folder
      puts cmd
      system(cmd)
      # update the files path information
      cmd = "install_name_tool -id @executable_path/../Frameworks/#{basename} #{app_frameworks}/#{basename}"
      #puts cmd
      system(cmd)
      relocate_lib(lib)
      @installed_list[lib] = true
    end
    yield lib, basename
  end

  def nonsystem_libs(libs)
    changelibs = []
    for lib in libs do
      dirname = File.dirname(lib)
      changelibs << lib if @libpaths.include?(dirname)
    end
    changelibs
  end

  def install_binary(binary_path)
    puts "Installing #{binary_path}"
    libs = bindeps(binary_path)

    changelibs = nonsystem_libs(libs)

    puts changelibs.inspect

    # set the bin path for the current binary install
    @app_binpath = "#{@app_macos}/#{File.basename(binary_path)}"

    # copy the executable into place
    system("cp #{binary_path} #{app_binpath}")

    for lib in changelibs do
      install_lib(lib) do|lib, basename|
        # update the executable information
        cmd = "install_name_tool -change #{lib} @executable_path/../Frameworks/#{basename} #{app_binpath}"
        puts cmd
        system(cmd)
      end
    end
  end

  def execute
    packages = ['gtk+-2.0', 'gthread-2.0', 'libglade-2.0']
    libpaths = []
    for pkg in packages do
      libpaths << `pkg-config #{pkg} --libs-only-L`.gsub(/-L/,'').strip
    end
    libpaths.uniq!
    @libpaths = libpaths
    install_binary("workqviz")
    install_binary("/opt/local/bin/pango-querymodules")
  end
end

class PangoPatch < Installer
  def patch
    libpath = `pkg-config pango --libs-only-L`.gsub(/-L/,'').strip
    system("cp #{libpath}/pango/1.6.0/modules/* #{app_resources}/pango/modules/")

    system("cp #{libpath}/../etc/pango/pango.modules #{app_resources}/etc/pango/"
    system("cp #{libpath}/../etc/pango/pangorc #{app_resources}/etc/pango/"
  end
end

installer = Installer.new
installer.execute

pango = PangoPatch.new
pango.patch
