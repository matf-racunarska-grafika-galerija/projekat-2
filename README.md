# MATF RG ~ ***Rural Mysteries***
______________________
Projekat napravljen za potrebe kursa *Računarska grafika*, na Matematičkom fakultetu u Beogradu. <br>
Korišćen je skelet projekta preuzet sa `https://github.com/matf-racunarska-grafika/project_base.git`. <br>
Pravljeno uz pomoć materijala sa [learnopengl](https://learnopengl.com/). <br>
<br>
Autori: 
- Vladimir Jovanović 96/2019
- Relja Pešić 73/2019


## Uputstvo pre izvršavanja
1. `git clone https://github.com/rexi12345/projekat.git`
2. CLion -> Open -> path/to/my/project_base
3. Main se nalazi u src/main.cpp
4. ALT+SHIFT+F10 -> project_base -> run

## Opis

*Stavljate se u ulogu detektiva koji je pozvan da istraži čudne fenomene u obližnjem selu.* <br>
*Naoružani samo baterijskom lampom, rešili ste da bacite svetlo na istinu. Čitavo mesto deluje pusto,* <br>
*ali ipak imate osećaj da niste sami...*

------------
Program se sastoji iz dve faze: 
1. ***Intro*** faze, u kojoj se simulira dolazak detektiva kolima do lokacije, i
2. faze ***istraživanja***, u kojoj se možete slobodno kretati po sceni.

U ***intro*** fazi, predstavljeno je renderovanje objekata i svetala pomoću [*Deferred Shading*](https://learnopengl.com/Advanced-Lighting/Deferred-Shading) tehnike i u toku ove faze <br>
onemogućeno je kretanje pomoću tastature i miša.

![screenshot 1](/resources/screenshots/ss1.png)

U fazi ***istraživanja***, moguće je slobodno kretanje po sceni, kao i podešavanje raznih efekata pomoću ImGUI menija.

![screenshot 2](/resources/screenshots/ss2.png)

## Uputstvo tokom izvršavanja
- Kontrole: standardno, `WASD` kretanje uz pomoć miša i tastature.
- Keybindings:
  - *General:*
      - `F1` - otvara ImGUI meni
      - `ESC` - prekida izvršavanje programa
  - *Post-processing efekti:*
      - `F2` - uključuje/isključuje [*Anti-Aliasing*](https://learnopengl.com/Advanced-OpenGL/Anti-Aliasing) efekat
      - `F3` - uključuje/isključuje *Grayscale* efekat
      - `H` - uključuje/isključuje [*HDR*](https://learnopengl.com/Advanced-Lighting/HDR) efekat
      - `B` - uključuje/isključuje [*Bloom*](https://learnopengl.com/Advanced-Lighting/Bloom) efekat
      - `Q` - smanjuje *exposure* parametar
      - `E` - povećava *exposure* parametar
  - *Ostala podešavanja:*
    - `N` - uključivanje/isključivanje ***Spectator*** moda:
      - `SPACE` - kretanje na gore
      - `SHIFT` - kretanje na dole
    - `RMB` - (desni klik) - uključivanje/isključivanje svetla na baterijskoj lampi
    - `C` - podešavanje vidljivosti kursora i zaključavanje kretanja kamere dok je otvoren meni


## Resursi
Svi resursi se mogu naći u ***resources*** direktorijumu. <br>
Modeli i njihove teksture su preuzeti sa [Free3D.com](https://free3d.com/) i [TurboSquid.com](https://www.turbosquid.com/), i neke od njih su doradjivane i/ili izmenjene <br>
pomoću [*Blender*-a](https://www.blender.org/) i [*GIMP-a*](https://www.gimp.org/). <br>
Skybox teksture su preuzete sa [*drajva*](https://drive.google.com/file/d/1-Cw7GqwP9GBVznwGJq5tYne5GEmyeA_X/view). (*Free CopperCube skyboxes.zip*)<br>

## Galerija

![screenshot 3](/resources/screenshots/ss3.png)

![screenshot 4](/resources/screenshots/ss4.png)

![screenshot 5](/resources/screenshots/ss5.png)

![screenshot 6](/resources/screenshots/ss6.png)
______________
## Detalji o kursu
Školska 2021 / 2022. godina <br>
Asistent: Marko Spasić <br>
Profesor: dr. Vesna Marinković
