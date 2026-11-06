

## Configuration
#Création du docker 
Création de docker à partir du docker file : 
```
docker build -t env-zephyr-espressif -f <Dockerfile.hardware> .  
```
D'abord trouver le docker file décrivant le docker. 
Puis lancer : 
```
docker run --rm -it -p 2222:22 -p 8800:8800 -v "${PWD}\workspace:/workspace" -w /workspace env-zephyr-espressif
```
Créer un localhost dans Visual Studio pour se connecter en ssh : 
```
root@localhost:2222
```
Une fois connectée, rentrer le mot de passe et TADAM on est dedans. 

## Flash et install 
#Simulation 
```
west build -p always -b native_sim -- -DDTC_OVERLAY_FILE=boards/nativesim.overlay
west build -t run
```
#Hardware 
```
west build -p always -b native_sim -- -DDTC_OVERLAY_FILE=boards/nativesim.overlay
```
A partir de là, plusieurs possibilités en fonction du hardware. 
Copier le fichier .elf ou .hex en local puis lancer avec CubeProgrammer par exemple : 

```
docker cp <container_id>:/workspace/apps/09_demo_gpio_interrupt/build/zephyr/zephyr.hex .
& "C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe" -c port=SWD -w zephyr.elf 0x08000000
```
Soit en utilisant west avec OpenCD
```
west flash --runner openocd
```
##Test 
#Test simple via serial.
Envoie de valeurs plusieurs options. Après avoir identifié "uart connected to pseudotty: /dev/pts/3" type. On peut envoyer des données soit en se connectant au pts via 
```
screen /dev/pts/3
```
Ou plus directement 
```
printf '...' > /dev/pts/3

```
#Test hardware
```
python -m serial.tools.miniterm "<PORT>" 115200 ‎
```
## Debug 
#GDB 
```
west build -b native_sim . --pristine -- -DCONFIG_NO_OPTIMIZATIONS=y
gdb ./build/zephyr/zephyr.exe
```
Puis utiliser utiliser les commandes telles que

```
(gdb) break /full/path/to/src/main.c:33    # Break at k_msleep (after led_state is set)
(gdb) run
(gdb) print led_state                       # Shows 0 or 1 (OFF or ON)
(gdb) print led                             # Inspect LED gpio_dt_spec
(gdb) print led.port->name                  # Shows "gpio_emul"
(gdb) continue                              # led_state will toggle
(gdb) print led_state                       # Now shows opposite value
(gdb) quit
```
# Renode
## Création d'une machine
En charge un fichier binaire
```
mach create
machine LoadPlatformDescription @platforms/cpus/miv.repl
```
Une fois que la machine est créée, on peut charger le binaire via : 
```
sysbus LoadELF @my-project.elf
```

## Description de la plateforme
```
```





