from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.scm import Git
from conan.tools.files import copy
from os import getenv
from random import getrandbits
from shutil import copytree

class StormEngine(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    # build options provided by CMakeLists.txt that are used in conanfile.py
    options = {
        "output_directory": ["ANY"],
        "watermark_file": ["ANY"],
        "crash_reports": [True, False],
        "steam": [True, False],
        "conan_sdl": [True, False]
    }

    # dependencies used in deploy binaries
    # conan-center
    requires = [
        "zlib/1.3.1",
        "spdlog/1.13.0",
        "fast_float/8.0.2",
        "mimalloc/2.1.7",
        "sentry-native/0.6.5",
        "tomlplusplus/3.3.0",
        "nlohmann_json/3.11.2",
        "imgui/1.90-docking",
        "cli11/2.3.2",
        "ms-gsl/4.1.0",
        # gitlab.com/piratesahoy
        "directx/jun10+9.29.1962.1",
        "fmod/2.02.05+1@piratesahoy+storm-engine/stable"
    ]

    # optional dependencies
    def requirements(self):
        if self.settings.os == "Windows":
            # conan-center
            self.requires("7zip/19.00")
        else:
            # conan-center
            self.requires("openssl/1.1.1n")#fix for error: 'sentry-crashpad/0.4.13' requires 'openssl/1.1.1n' while 'pulseaudio/14.2' requires 'openssl/1.1.1q'
            self.options["sdl"].nas = False #fix for https://github.com/conan-io/conan-center-index/issues/16606 - error: nas/1.9.4: Invalid ID: Recipe cannot be built with clang
            self.options["libsndfile"].with_mpeg= False #fix for 0a12560440ac9f760670829a1cde44b787f587ad/src/src/libmpg123/mpg123lib_intern.h:346: undefined reference to `__pow_finite'
        if self.options.steam:
            self.requires("steamworks/1.5.1@storm/prebuilt")
        if self.options.conan_sdl:
            self.requires("sdl/2.32.2")

        self.test_requires("catch2/3.9.1")

    generators = "CMakeDeps"

    default_options = {
        "crash_reports": False,
        "steam": False,
        "conan_sdl": True,
        "sentry-native/*:backend": "crashpad",
        "mimalloc/*:shared": True,
        "mimalloc/*:override": True
    }

    def validate(self):
        check_min_cppstd(self, "20")

    def generate(self):
        self.__dest = str(self.options.output_directory) + "/" + getenv("CONAN_IMPORT_PATH", "bin")
        self.__install_folder("/src/techniques", "/resource/techniques")
        self.__install_folder("/src/libs/shared_headers/include/shared", "/resource/shared")

        self.__copy_imgui_binding("imgui_impl_sdl2.cpp")
        self.__copy_imgui_binding("imgui_impl_sdl2.h")
        self.__copy_imgui_misc("imgui_stdlib.cpp")
        self.__copy_imgui_misc("imgui_stdlib.h")
        if self.settings.os == "Windows":
            self.__copy_imgui_binding("imgui_impl_dx9.cpp")
            self.__copy_imgui_binding("imgui_impl_dx9.h")

        if self.settings.os == "Windows":
            if self.settings.build_type == "Debug":
                self.__install_lib("fmodL.dll", self.dependencies["fmod"])
            else:
                self.__install_lib("fmod.dll", self.dependencies["fmod"])

            self.__install_bin("crashpad_handler.exe", self.dependencies["sentry-native"])
            if self.options.crash_reports:
                self.__install_bin("7za.exe", self.dependencies["7zip"])
            if self.options.steam:
                self.__install_lib("steam_api64.dll", self.dependencies["steamworks"])

            self.__install_bin("mimalloc-redirect.dll", self.dependencies["mimalloc"])
            if self.settings.build_type == "Debug":
                self.__install_bin("mimalloc-debug.dll", self.dependencies["mimalloc"])
            else:
                self.__install_bin("mimalloc.dll", self.dependencies["mimalloc"])

        else: # not Windows
            if self.settings.build_type == "Debug":
                self.__install_lib("libfmodL.so.13", self.dependencies["fmod"])
            else:
                self.__install_lib("libfmod.so.13", self.dependencies["fmod"])

            self.__install_bin("crashpad_handler", self.dependencies["sentry-native"])
            #if self.options.steam:
            #    self.__install_lib("steam_api64.dll")#TODO: fix conan package and then lib name

            if self.settings.build_type == "Debug":
                self.__install_lib("libmimalloc-debug.so.2.0", self.dependencies["mimalloc"])
                self.__install_lib("libmimalloc-debug.so", self.dependencies["mimalloc"])
            else:
                self.__install_lib("libmimalloc.so.2.0", self.dependencies["mimalloc"])
                self.__install_lib("libmimalloc.so", self.dependencies["mimalloc"])

        self.__write_watermark()


    def __write_watermark(self):
        with open(str(self.options.watermark_file), 'w') as f:
            f.write("#pragma once\n#define STORM_BUILD_WATERMARK ")
            f.write(self.__generate_watermark())
            f.write("\n")

    def __get_branch(self, git: Git):
        return git.run("rev-parse --abbrev-ref HEAD")

    def __generate_watermark(self):
        git = Git(self, self.recipe_folder)
        try:
            if not git.is_dirty():
                return "%s(%s)" % (self.__get_branch(git), git.get_commit())
            else:
                return "%s(%s)-DIRTY(%032x)" % (self.__get_branch(git), git.get_commit(), getrandbits(128))
        except:
            return "unknown"

    def __install_bin(self, name, package):
        copy(self, pattern=name, dst=self.__dest, src=str(package.package_folder) + "/bin", keep_path=False)

    def __install_lib(self, name, package):
        copy(self, pattern=name, dst=self.__dest, src=str(package.package_folder) + "/lib", keep_path=False)

    def __install_folder(self, src, dst):
        copytree(self.recipe_folder + src, self.__dest + dst, dirs_exist_ok=True)

    def __copy_imgui_binding(self, name):
        imgui = self.dependencies["imgui"]
        self.output.info("res:{}".format(imgui.package_folder))
        copy(self, name, dst=str(self.options.output_directory) + "/imgui", src=f"{str(imgui.package_folder)}/res/bindings")

    def __copy_imgui_misc(self, name):
        imgui = self.dependencies["imgui"]
        copy(self, name, dst=str(self.options.output_directory) + "/imgui", src=f"{str(imgui.package_folder)}/res/misc/cpp")
