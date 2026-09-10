# TMC Practice — Guida in italiano

[English](README_EN.md) · [Deutsch](README_DE.md) · [Français](README_FR.md) · [Español](README_ES.md) · [Italiano](README_IT.md) · [日本語](README_JA.md)

TMC Practice aggiunge un menu di allenamento ed esplorazione a **The Legend of Zelda: The Minish Cap**.
La versione **v1.0.0** offre patch BPS separate per USA, Europa e Giappone.
Questa guida è tradotta; **il menu Practice rimane in inglese**.
La selezione delle lingue europee e i testi giapponesi del gioco sono conservati.
I nomi inglesi delle opzioni sono mantenuti qui sotto per ritrovarli nel gioco.

## 1. Download e installazione

1. Apri [TMC Practice v1.0.0](https://github.com/Nimcoz/TMC-Practice/releases/tag/v1.0.0).
2. Scarica una sola patch per la **regione della tua ROM originale**, non per la lingua di questa guida:
   - USA: `TMC-Practice-v1.0.0-USA.bps`.
   - Europa (inglese, francese, tedesco, spagnolo, italiano): `TMC-Practice-v1.0.0-Europe.bps`.
   - Giappone: `TMC-Practice-v1.0.0-Japan.bps`.
3. Crea una copia di sicurezza separata del tuo normale salvataggio di gioco.
4. Applica la patch BPS alla **ROM originale non modificata** con uno strumento compatibile con BPS.
5. Apri il file `.gba` ottenuto nell'emulatore e usa un salvataggio normale della stessa regione.
6. Premi **L + R + Select** per aprire o chiudere il menu. Una combinazione personalizzata già salvata ha la precedenza.

L'[elenco delle ROM compatibili](ROM_COMPATIBILITY.md) riporta le impronte esatte supportate.
Non ignorare gli errori di checksum. Non combinare patch, codici AR esterni o altre
modifiche Practice/No-Clip e non caricare stati dell'emulatore creati con versioni
precedenti. Non sono inclusi ROM, salvataggi o stati dell'emulatore.
Non viene distribuito un convertitore di salvataggi tra regioni diverse.

## 2. Comandi e impostazioni

- **Croce direzionale:** seleziona una riga; sinistra/destra cambia un valore modificabile.
- **A:** attiva/conferma. **B:** indietro/annulla.
- **Azioni distruttive:** segui la conferma sullo schermo; normalmente tieni premuti **L + R** e premi di nuovo **A** sulla riga di conferma.
- **I valori grezzi sono esadecimali.** Sinistra/destra cambia di uno; L/R cambia di `0x10` i campi da un byte compatibili.
- **SETTINGS:** temi, pulsanti e impostazioni persistenti del menu. Salvare queste impostazioni **non** salva automaticamente i progressi di gioco.

## 3. Orientarsi nel menu

| Menu | Funzione |
| --- | --- |
| PRACTICE | Cronometro e comandi di allenamento; tempo visibile durante il gioco |
| PLAYER / MOVEMENT | Link, movimento, No-Clip e telecamera |
| INVENTORY | Oggetti, equipaggiamento, bottiglie e singoli elementi |
| WORLD / WARP | Stanze, preferiti e replay dell'introduzione e del finale |
| FLAGS | Esaminare e modificare gli indicatori di stato del gioco |
| DEBUG | Informazioni di debug visibili durante il gioco |
| CHEATS | Risorse, Enemy Freeze, Infinite Time e altri trucchi |
| SETTINGS | Aspetto, comandi e preferenze salvate |
| ACTORS / OBJECTS | Elenco degli attori, generatore grezzo, blocco, rimozione e teletrasporto |

Le collezioni, i flag e l'azione **100%** dopo la conferma possono modificare i
progressi della storia. Fai un backup prima di sperimentare.
Le voci inutilizzate/Beta non sono contenuti Beta ricostruiti.

## 4. Aprire il menu fuori dal gioco normale

Sono supportate le schermate native dell'inventario, dialoghi/filmati, titolo/introduzione
e finale. Le azioni distruttive sul mondo richiedono gioco attivo e possono essere
disabilitate in queste schermate. Durante dissolvenze, trasferimenti di risorse o
scritture effettive del salvataggio EEPROM, il menu attende: non tutti i fotogrammi
di una transizione possono essere interrotti.

## 5. Rivedere l'introduzione o il finale

Durante il gioco normale, scegli **WORLD / WARP → STORY INTRO** oppure **ENDING / CREDITS**.
Premi **A**, poi ancora **A** per confermare. Non avviare un replay durante un
dialogo, un filmato o dall'inventario nativo.

**RETURN FROM REPLAY** termina il replay e ricarica la stanza originale. Tutti i
1.204 byte del salvataggio nativo vengono copiati in RAM e ripristinati al ritorno.
Gli attori temporanei della stanza non vengono ripristinati come in uno stato
dell'emulatore. Le altre modifiche, i trucchi e il cronometraggio vengono sospesi
temporaneamente. Il replay del finale ritorna prima della normale richiesta di
salvataggio; il finale normale conserva il suo comportamento originale.
Riavviare l'emulatore fa perdere il punto di ritorno temporaneo in RAM.

## 6. Strumenti per gli attori

**ACTORS / OBJECTS → RAW NATIVE SPAWNER** espone categorie e nomi nativi e i valori
grezzi kind/ID/type/type2/timer/subtimer/flags/parent/layer. **QUICK SPAWN PRESETS**
non limita la modalità grezza. Non c'è un elenco restrittivo di stanze/piani/varianti;
restano rifiutate le routine native mancanti e le richieste con i pool di attori
già pieni. Sono etichettati 546 slot nativi e 118 tipi di oggetti a terra;
25 voci restano intenzionalmente **UNKNOWN**.

In **ROOM ACTOR LIST**, seleziona un attore per bloccarlo, rimuoverlo o spostarlo.
**LINK TO ACTOR** teletrasporta Link verso l'attore; **ACTOR TO LINK** porta l'attore
da Link, nella stanza attuale senza cambiare stanza. I gestori senza struttura XYZ
comune mostrano **NO GENERIC XYZ**: lo spostamento generico non è disponibile,
ma blocco e rimozione restano possibili. Blocca prima l'attore se la sua IA lo sposta di nuovo.

Un secondo Link, boss/tipi incompatibili o dipendenze mancanti da stanze, script
o attori genitori possono causare arresti o blocchi del gioco. Rimuovere un boss
**non equivale** a sconfiggerlo. Se necessario, ricarica senza salvare.

## 7. Enemy Freeze, Break Free e Infinite Time

- **Enemy Freeze:** include boss e parti visibili del corpo prive di collisione propria. I proiettili di altre categorie possono richiedere il blocco individuale.
- **Break Free:** chiude il testo attivo attraverso lo stato di chiusura nativo e restituisce il controllo al giocatore. Non annulla script già eseguiti; script successivi possono riprendere il controllo.
- **Infinite Time:** comprende il conto alla rovescia delle galline di Anju, quello del Castello di Hyrule oscuro, l'attivazione temporanea degli interruttori a forma di occhio e la durata di amuleti/pozioni della fortuna già attivi. Non blocca tutti i timer, le animazioni o i filmati. Disattivalo per consentire la valutazione dei conti alla rovescia, le ricompense e la conclusione degli effetti temporanei.

## 8. Test, problemi e collaborazione

Questa versione ha superato **166 suite automatizzate di mGBA: 53 USA + 113 EU/JP**.
Il tester del progetto ha anche confermato apertura del menu e No-Clip nel suo
emulatore. Non è stata documentata una verifica completa su Android per ogni regione;
non sono stati eseguiti test su una GBA reale. Vedi le [verifiche](VERIFICATION.md).

Segnala i problemi in [Issues](https://github.com/Nimcoz/TMC-Practice/issues), indicando
regione, versione della patch, emulatore e versione, passaggi e, se utile, uno screenshot.
Non caricare ROM, salvataggi privati o credenziali. I contributi al progetto ufficiale
richiedono un accordo preventivo con Nimcoz; le segnalazioni di bug no. Vedi le
[regole di collaborazione](../CONTRIBUTING.md) e i [crediti e diritti](../THIRD_PARTY_NOTICES.md).
