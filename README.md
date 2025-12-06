# Tamandutech Core Collections (TTCC)

## Downloads (Binários Pré-Compilados)
 Baixe o pacote completo (contém `ttesp32` e `ttds4`) para o seu sistema:

 [![Baixar Windows][baixar_windows_icon]][baixar_windows_zip]
 [![Baixar MacOS][baixar_macos_icon]][baixar_macos_zip]
 [![Baixar Linux][baixar_linux_icon]][baixar_linux_zip]

---

## Compilar e Instalar (Source)
 Caso prefira compilar manualmente.

 **Dependências:**
 * **Windows (MSYS2):** 
 ```bash
 pacman -S base-devel
 pacman -S libusb
 ```

 * **MacOS (Darwin):** 
 ```bash
 brew install pkg-config
 brew install libusb
 ```

 * **Linux (Arch):** 
 ```bash
 sudo pacman -S base-devel
 sudo pacman -S libusb
 ```

 * **Linux (Debian):** 
 ```bash
 sudo apt install build-essential
 sudo apt install libusb-1.0-0-dev
 ```

 ```bash
 cd "/tmp"
 git clone "[https://github.com/GabrielFrigo4/TTCC.git](https://github.com/GabrielFrigo4/TTCC.git)"
 cd TTCC
 make
 sudo make install
 cd ~
 sudo rm -r "/tmp/TTCC"
 ```

---

## TTESP32
 Ferramenta para leitura do MAC Address de dispositivos ESP32.

 **Uso Básico:**
 ```bash
 ttesp32 -r
 ```

 **Workflow (Ler ESP32 -> Gravar no DS4):**
 ```bash
 ttesp32 -r | sudo ttds4 -w
 ```

## TTDS4
 Ferramenta para leitura e escrita do "Master MAC Address" em controles DualShock 4 (via USB).

 **Ler MAC Atual:**
 ```bash
 ttds4 -r
 ```

 **Escrever Novo MAC:**
 ```bash
 sudo ttds4 -w AA:BB:CC:DD:EE:FF
 ```

[baixar_windows_zip]: https://github.com/GabrielFrigo4/TTCC/releases/download/latest/windows.zip
[baixar_macos_zip]: https://github.com/GabrielFrigo4/TTCC/releases/download/latest/macos.zip
[baixar_linux_zip]: https://github.com/GabrielFrigo4/TTCC/releases/download/latest/linux.zip

[baixar_windows_icon]: https://img.shields.io/badge/Windows-Baixar_ZIP-blue?style=for-the-badge&logo=data:image/svg%2Bxml;base64,PHN2ZyB3aWR0aD0iMjU2cHgiIGhlaWdodD0iMjU2cHgiIHZpZXdCb3g9IjAgMCAyNTYgMjU2IiB2ZXJzaW9uPSIxLjEiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyIgeG1sbnM6eGxpbms9Imh0dHA6Ly93d3cudzMub3JnLzE5OTkveGxpbmsiIHByZXNlcnZlQXNwZWN0UmF0aW89InhNaWRZTWlkIj4gICAgPHRpdGxlPldpbmRvd3M8L3RpdGxlPiAgICA8Zz4gICAgICAgIDxwYXRoIGQ9Ik0wLDIuODQyMTcwOTRlLTE0IEwxMjEuMzI4OTksMi44NDIxNzA5NGUtMTQgTDEyMS4zMjg5OSwxMjEuMzI4OTkgTDAsMTIxLjMzODk5IEwwLDIuODQyMTcwOTRlLTE0IFogTTEzNC42NzEwMSwyLjg0MjE3MDk0ZS0xNCBMMjU2LDIuODQyMTcwOTRlLTE0IEwyNTYsMTIxLjMzODk5IEwxMzQuNjcxMDEsMTIxLjMzODk5IEwxMzQuNjcxMDEsMi44NDIxNzA5NGUtMTQgWiBNMCwxMzQuNjcxMDEgTDEyMS4zMjg5OSwxMzQuNjcxMDEgTDEyMS4zMjg5OSwyNTYgTDAsMjU2IEwwLDEzNC42NzEwMSBaIE0xMzQuNjcxMDEsMTM0LjY3MTAxIEwyNTYsMTM0LjY3MTAxIEwyNTYsMjU2IEwxMzQuNjcxMDEsMjU2IEwxMzQuNjcxMDEsMTM0LjY3MTAxIFoiIGZpbGw9IiMwMDc4RDQiLz4gICAgPC9nPjwvc3ZnPg==
[baixar_macos_icon]: https://img.shields.io/badge/MacOS-Baixar_ZIP-white?style=for-the-badge&logo=apple
[baixar_linux_icon]: https://img.shields.io/badge/Linux-Baixar_ZIP-red?style=for-the-badge&logo=linux
