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
 [![Baixar TTESP32][baixar_ttesp32_icon]][baixar_ttesp32]

 Print ESP32 Mac Address
 ```cmd
 ttesp32
 ```

 Write ESP32 Mac Address on PS4 Controller Master Mac Address
 ```cmd
 ttesp32 | sudo ttds4 -w
 ```

## TTDS4
 [![Baixar TTDS4][baixar_ttds4_icon]][baixar_ttds4]

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

[baixar_ttesp32]: https://github.com/GabrielFrigo4/TTCC/releases/download/latest/ttesp32
[baixar_ttds4]: https://github.com/GabrielFrigo4/TTCC/releases/download/latest/ttds4
[baixar_ttesp32_icon]: https://img.shields.io/badge/TTESP32-Baixar-red?style=for-the-badge&logo=adobeacrobatreader
[baixar_ttds4_icon]: https://img.shields.io/badge/TTDS4-Baixar-red?style=for-the-badge&logo=adobeacrobatreader
