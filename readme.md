# Building & Running

## Requirements

Before building the project, install the required dependencies.

### Linux (Tested on Fedora)

#### Build Tools

```bash
sudo dnf install make gcc-c++
```

#### Libraries

```bash
sudo dnf install \
    glm-devel \
    vulkan-loader-devel \
    vulkan-headers \
    vulkan-validation-layers \
    glfw-devel
```

#### DirectX Shader Compiler (DXC)

Download the latest Linux release from:

https://github.com/microsoft/DirectXShaderCompiler/releases

Extract the archive:

```bash
tar -xf linux_dxc*.tar.gz
```

Install the compiler:

```bash
sudo cp bin/dxc /usr/local/bin/
sudo cp lib/libdxcompiler.so* /usr/local/lib/
```

---

## Build

From the project root:

```bash
cd ProjectFolder
```

Generate the project files:

```bash
./scripts/generateProjects.sh
```

Compile the shaders:

```bash
./scripts/compileShaders.sh
```

Build the project:

```bash
make config=debug
```

---

## Run

Start the game:

```bash
./game/bin/Debug-linux-x86_64/game/game
```

---

## Windows

>  Coming soon.