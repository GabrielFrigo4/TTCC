# Tamandutech Core Collections (TTCC)
 Install TTCC
 ```cmd
 cd "/tmp"
 git clone "https://github.com/GabrielFrigo4/TTCC.git"
 cd TTCC
 make -j$(nproc)
 sudo make install
 sudo rm -r "/tmp/TTCC"
 ```

## TTESP32
 [![Baixar TTESP32][baixar_linux_icon]][baixar_ttesp32_linux]
 [![Baixar TTESP32][baixar_win_icon]][baixar_ttesp32_win]

 Print ESP32 Mac Address
 ```cmd
 ttesp32
 ```

 Write ESP32 Mac Address on PS4 Controller Master Mac Address
 ```cmd
 ttesp32 | sudo ttds4 -w
 ```

## TTDS4
 [![Baixar TTDS4][baixar_linux_icon]][baixar_ttds4_linux]
 [![Baixar TTDS4][baixar_win_icon]][baixar_ttds4_win]

 Print PS4 Controller Master Mac Address
 ```cmd
 ttds4 -r
 ```

 Write PS4 Controller Master Mac Address
 ```cmd
 sudo ttds4 -w AA:BB:CC:DD:EE:FF
 ```

 Write ESP32 Mac Address on PS4 Controller Master Mac Address
 ```cmd
 ttesp32 | sudo ttds4 -w
 ```

[baixar_ttesp32_linux]: https://github.com/GabrielFrigo4/TTCC/releases/download/latest/ttesp32
[baixar_ttds4_linux]: https://github.com/GabrielFrigo4/TTCC/releases/download/latest/ttds4
[baixar_ttesp32_win]: https://github.com/GabrielFrigo4/TTCC/releases/download/latest/ttesp32.exe
[baixar_ttds4_win]: https://github.com/GabrielFrigo4/TTCC/releases/download/latest/ttds4.exe
[baixar_linux_icon]: https://img.shields.io/badge/Linux-Baixar-red?style=for-the-badge&logo=adobeacrobatreader
[baixar_win_icon]: https://img.shields.io/badge/Win-Baixar-blue?style=for-the-badge&logo=adobeacrobatreader
