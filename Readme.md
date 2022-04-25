# IPK - projekt 1
## Autor
- Jiří Mládek (xmlade01)


## Hodnocení
- 20/20

## Popis projektu

Jedná se o server v jazyce C komunikující prostřednictvím protokolu HTTP, který poskytuje informace o systému. Konkrétně doménové jméno, informace o CPU a aktuální zátěž procesoru. Server naslouchá na zadaném portu v argumentu programu. Podle tvaru url vrací požadované informace.

## Způsob spuštění a příklady použítí projektu

Pro přeložení je potřeba gcc kompilátor. 
Přeložíme pomocí Makefile zadáním příkazu 'make' do terminálu, vytvoří se spustitelný soubor hinfosvc.

```
$ make
```

První možnost je taková, že spustíme program s argumentem označující lokální port na kterém server naslouchá požadavkům. 

```
$ ./hinfosvc 12345
```

Poté zadáme do prohlížeče jednu z následujících url:

* http://localhost:12345/hostname  - získáme jméno počítače včetně domény
* http://localhost:12345/cpu-name  - získáme informace o procesoru
* http://localhost:12345/load      - získáme informace o zátěži CPU


Server je možné ukončit pomocí CTRL+C.

Druhou možností je zaslání příkazu GET přímo z termináĺu, kde ihned za spuštěním programu následuje znak '&' a po něm druh nástroje (např. wget nebo curl) a jedna z url.

```
$ ./hinfosvc 12345 &
curl http://localhost:12345/hostname
curl http://localhost:12345/cpu-name
curl http://localhost:12345/load
```

## Použíté zdroje

https://stackoverflow.com/questions/23367857/accurate-calculation-of-cpu-usage-given-in-percentage-in-linux