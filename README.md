# Demo X11 Client

This is an X11-based remote desktop client built with FreeRDP.

## Setup Instructions

### 1. Install vcpkg

First, clone the vcpkg package manager:

```bash
git clone https://github.com/Microsoft/vcpkg.git
```

Navigate to the vcpkg directory and run the bootstrap script:

```bash
cd vcpkg
./bootstrap-vcpkg.sh
```

### 2. Set Environment Variable

Create the `VCPKG_ROOT` environment variable pointing to your vcpkg installation:

```bash
export VCPKG_ROOT=/path/to/your/vcpkg
```

Add this to your shell profile (`.bashrc`, `.zshrc`, etc.) to make it permanent:

```bash
echo 'export VCPKG_ROOT=/path/to/your/vcpkg' >> ~/.bashrc
source ~/.bashrc
```

### 3. Install FreeRDP

Install FreeRDP using vcpkg:

```bash
vcpkg install freerdp
```

### 4. Install X11 Dependencies

Install the required X11 libraries on your Linux system:

```bash
sudo apt update
sudo apt install -y \
    libx11-dev \
    libxext-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxfixes-dev \
    libxi-dev \
    libxss-dev \
    libxkbfile-dev \
    build-essential \
    cmake \
    pkg-config
```

### 5. Build the Project

Create a build directory and compile the project:

```bash
mkdir build
cd build
cmake ..
make
```

### 6. Run the Client

Execute the demo with your connection parameters:

```bash
./demo_x11 /v:<your-server-ip> /u:<username> /p:<password>
```

Example:
```bash
./demo_x11 /v:xxx.xxx.x.xx /u:YourUsername /p:YourPassword
```

## Notes

- Make sure your vcpkg toolchain is properly configured
- Ensure all X11 dependencies are installed before building
- The client connects to Windows Remote Desktop services via FreeRDP