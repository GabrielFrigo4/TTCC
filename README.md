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
 Print ESP32 Mac Address
 ```cmd
 ttesp32
 ```

 Write ESP32 Mac Address on PS4 Controller Master Mac Address
 ```cmd
 ttesp32 | sudo ttds4 -w
 ```

## TTDS4
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
