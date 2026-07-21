from conan import ConanFile
from conan.tools.cmake import cmake_layout


class RobotDesignCopilotDependencies(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "CMakeToolchain", "VirtualRunEnv"

    def requirements(self):
        # Pinocchio 3.8.0 is the newest ConanCenter release with prebuilt
        # packages for Windows, macOS, and Linux. Its tested Eigen line is 3.4.
        self.requires(
            "pinocchio/3.8.0",
            options={"with_collision_support": False},
        )
        self.requires(
            "ceres-solver/2.2.0",
            options={
                "shared": False,
                "use_glog": False,
                "use_custom_blas": True,
                "use_eigen_sparse": True,
                "use_schur_specializations": True,
            },
        )
        self.requires("eigen/3.4.1", override=True)
        self.requires("nlohmann_json/3.12.0")

    def layout(self):
        cmake_layout(self)
