import os
import platform
import subprocess
from pathlib import Path

# Handles windows, mac, and anything based on debian
def check_if_installed():
    opencv = False
    tensorflow = False
    if platform.system() == "Windows":
        user_path = ""
        user_opencv_dir = ""
        # Get the user PATH environment variable, not system
        result = subprocess.run('reg query "HKCU\\Environment" /v PATH', capture_output=True, text=True, shell=True)
        if result.returncode == 0:
            lines = result.stdout.splitlines()
            for line in lines:
                if "PATH" in line:
                    user_path = line.split("REG_SZ")[-1].strip()
        result = subprocess.run('reg query "HKCU\\Environment" /v OPENCV_DIR', capture_output=True, text=True, shell=True)
        if result.returncode == 0:
            lines = result.stdout.splitlines()
            for line in lines:
                if "OPENCV_DIR" in line:
                    user_opencv_dir = line.split("REG_SZ")[-1].strip()
        if (user_opencv_dir != "" and user_path.find("opencv\\build\\x64\\vc16\\bin") != -1) or (os.environ.get("OPENCV_DIR") and os.environ["PATH"].find("opencv\\build\\x64\\vc16\\bin") != -1):
            opencv = True
        if user_path.find("tensorflow\\include") != -1 or os.environ["PATH"].find("tensorflow\\include") != -1:
            tensorflow = True
    elif platform.system() == "Darwin":
        if subprocess.call(["brew", "list", "opencv"]) == 0:
            opencv = True
        if subprocess.call(["ldd", "/usr/local/lib/libtensorflow.so"]) == 0:
            tensorflow = True
    else:
        if subprocess.call(["dpkg", "-s", "libopencv-dev"]) == 0:
            opencv = True
        if subprocess.call(["ldd", "/usr/local/lib/libtensorflow.so"]) == 0:
            tensorflow = True
    return (opencv, tensorflow)

def install_packages():
    if platform.system() == "Windows":
        subprocess.check_call(["curl", "-L", "https://aka.ms/vs/17/release/vc_redist.x64.exe", "-o", "vc_redist.x64.exe"])
        subprocess.call(["vc_redist.x64.exe", "/install", "/quiet", "/norestart"])
        os.remove("vc_redist.x64.exe")

    (opencv, tensorflow) = check_if_installed()
    if opencv and tensorflow:
        print("OpenCV and TensorFlow are already installed.")
        input("Press Enter to exit...")
        return

    # Define installation directories
    install_dir = Path.home()
    os.makedirs(install_dir, exist_ok=True)

    paths = []
    if not opencv:
        print("Installing OpenCV...")
        # Install OpenCV
        if platform.system() == "Windows":
            opencv_url = "https://github.com/opencv/opencv/releases/download/4.10.0/opencv-4.10.0-windows.exe"
            opencv_filename = os.path.join(install_dir, opencv_url.split("/")[-1])
            subprocess.check_call(["curl", "-L", opencv_url, "-o", opencv_filename])
            subprocess.check_call([opencv_filename, "/S", "/D=" + os.path.join(install_dir, "opencv")])
            opencv_path = os.path.join(install_dir, "opencv", "build", "x64", "vc16", "bin")
            opencv_dir = os.path.join(install_dir, "opencv")
            print(f"OpenCV installed to {opencv_dir}")
            paths.append(opencv_path)
        elif platform.system() == "Darwin":
            subprocess.check_call(["brew", "install", "opencv"])
        else:
            subprocess.check_call(["sudo", "apt", "install", "-y", "libopencv-dev"])

    if not tensorflow:
        print("Installing TensorFlow...")
        # Install TensorFlow C API
        if platform.system() == "Windows":
            tf_url = "https://storage.googleapis.com/tensorflow/libtensorflow/libtensorflow-gpu-windows-x86_64-2.10.0.zip"
        elif platform.system() == "Darwin":
            tf_url = "https://storage.googleapis.com/tensorflow/libtensorflow/libtensorflow-cpu-darwin-arm64-2.18.0.tar.gz"
        else:
            tf_url = "https://storage.googleapis.com/tensorflow/libtensorflow/libtensorflow-gpu-linux-x86_64-2.18.0.tar.gz"

        tf_filename = os.path.join(install_dir, tf_url.split("/")[-1])
        subprocess.check_call(["curl", "-L", tf_url, "-o", tf_filename])
        if tf_filename.endswith(".zip"):
            os.makedirs(os.path.join(install_dir, "tensorflow"), exist_ok=True)
            subprocess.check_call(["tar", "-xf", tf_filename, "-C", os.path.join(install_dir, "tensorflow")])
            print(f"TensorFlow installed to {os.path.join(install_dir, 'tensorflow')}")
            paths.append(os.path.join(install_dir, "tensorflow", "lib"))
            paths.append(os.path.join(install_dir, "tensorflow", "include"))
        else:
            print(f"installing tensorflow to /usr/local/")
            subprocess.check_call(["sudo", "tar", "-xzf", tf_filename, "-C", "/usr/local/"])
            subprocess.check_call(["ldconfig"])

    if not tensorflow or not opencv:
        print("Installation complete.")
        if platform.system() != "Windows":
            input("Press Enter to exit...")
            exit(0)
        if not opencv:
            print(f"Add the environment variable OpenCV_DIR with the value {os.path.join(install_dir, 'opencv\\build')}")
        print("Please add the following paths to the PATH environment variable:")
    else:
        print("No packages were installed.")

    for path in paths:
        print(path)
    input("Press Enter to exit...")

install_packages()
