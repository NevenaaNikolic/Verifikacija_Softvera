# Projekat za kurs Verifikacija Softvera
# Tema: Apstraktna interpretacija
Master Studije

## Autori:
* Nevena Nikolić 1021/2018
* Milan Čugurović 1009/2018
* Luka Kalinić 1058/2018
* Aleksandar Anžel 1025/2018

### Za pokretanje programa, potrebno je:
* clang-7
* llvm-7
* cmake

### Instalacija potrebnih alata:
* sudo apt install ime_alata (**za verzije bazirane na Ubuntu sistemu**)

### Test primeri su smešteni u folderu 'test'
* Za kreiranje novih test primera, postupak je sledeći:
	* Pozicionirati se u direktorijum 'test' i u njemu iskopirati željeni .cpp fajl
	* Pokrenuti komandu: clang++-7 -O3 -emit-llvm ime_primera.cpp -c
	* Kao rezultat dobijamo ime_primera.bc fajl
	
### Prevođenje i pokretanje programa:
* Pozicioniranje u izvorni direktorijum
* Pokretanje komande: mkdir build
* Pokretanje komande: cd build
* Pokretanje komande: cmake ..
* Pokretanje komande: make
* Pokretanje komande: cd ..
* Pokretanje komande: opt-7 -load build/src/AI_INTERVALI.so -AI-PROLAZ test/ime_primera.bc
