# TMC Practice — Guide français

[English](README_EN.md) · [Deutsch](README_DE.md) · [Français](README_FR.md) · [Español](README_ES.md) · [Italiano](README_IT.md) · [日本語](README_JA.md)

TMC Practice ajoute un menu d'entraînement et d'exploration à **The Legend of Zelda: The Minish Cap**.
La version **v1.0.0** propose des patchs BPS distincts pour les versions USA, Europe et Japon.
Ce guide est traduit ; **le menu Practice lui-même reste en anglais**.
Le choix des langues européennes et les textes japonais du jeu sont conservés.
Les noms anglais des options sont conservés ci-dessous pour les retrouver dans le jeu.

## 1. Téléchargement et installation

1. Ouvrez [TMC Practice v1.0.0](https://github.com/Nimcoz/TMC-Practice/releases/tag/v1.0.0).
2. Téléchargez un seul patch correspondant à la **région de votre ROM d'origine**, et non à la langue de ce guide :
   - USA : `TMC-Practice-v1.0.0-USA.bps`.
   - Europe (anglais, français, allemand, espagnol, italien) : `TMC-Practice-v1.0.0-Europe.bps`.
   - Japon : `TMC-Practice-v1.0.0-Japan.bps`.
3. Faites une copie de secours séparée de votre sauvegarde normale du jeu.
4. Appliquez le patch BPS à votre **ROM d'origine non modifiée** avec un outil compatible BPS.
5. Ouvrez le fichier `.gba` obtenu dans votre émulateur et utilisez une sauvegarde normale de la même région.
6. Appuyez sur **L + R + Select** pour ouvrir ou fermer le menu. Un raccourci personnalisé déjà enregistré est prioritaire.

La [liste de compatibilité des ROMs](ROM_COMPATIBILITY.md) indique les empreintes exactes prises en charge.
N'ignorez pas une erreur de somme de contrôle. Ne cumulez pas les patchs, les codes AR
externes ou d'autres modifications Practice/No-Clip. Ne chargez pas d'états instantanés
d'émulateur provenant d'anciennes versions. Aucune ROM, sauvegarde ou sauvegarde instantanée
n'est fournie. Aucun convertisseur de sauvegardes entre régions n'est distribué.

## 2. Commandes et réglages

- **Croix directionnelle :** choisir une ligne ; gauche/droite modifie une valeur éditable.
- **A :** activer/confirmer. **B :** revenir/annuler.
- **Actions destructrices :** suivez la confirmation affichée ; généralement, maintenez **L + R** et appuyez de nouveau sur **A** sur la ligne de confirmation.
- **Les valeurs brutes sont hexadécimales.** Gauche/droite modifie de un ; L/R modifie de `0x10` les champs d'un octet concernés.
- **SETTINGS :** thèmes, raccourcis et réglages persistants du menu. Enregistrer ces réglages ne sauvegarde **pas** automatiquement la progression du jeu.

## 3. Se repérer dans le menu

| Menu | Fonction |
| --- | --- |
| PRACTICE | Chronomètre d'entraînement et commandes ; affichage du temps en jeu |
| PLAYER / MOVEMENT | Link, déplacements, No-Clip et caméra |
| INVENTORY | Objets, équipement, flacons et éléments individuels |
| WORLD / WARP | Salles, favoris et relecture de l'introduction ou de la fin |
| FLAGS | Consulter et modifier les indicateurs d'état du jeu |
| DEBUG | Informations de débogage affichées en jeu |
| CHEATS | Ressources, Enemy Freeze, Infinite Time et autres codes de triche |
| SETTINGS | Apparence, commandes et préférences enregistrées |
| ACTORS / OBJECTS | Liste des acteurs, génération brute, gel, suppression et téléportation |

Les collections, les indicateurs et l'action **100%** après confirmation peuvent
modifier la progression de l'histoire. Faites une copie de secours avant vos essais.
Les entrées inutilisées/Beta ne sont pas du contenu Beta reconstitué.

## 4. Ouvrir le menu en dehors du jeu normal

Le menu prend en charge les écrans d'inventaire natifs, dialogues/cinématiques,
écrans titre/introduction et de fin. Les actions destructrices sur le monde exigent
une partie active et peuvent être désactivées dans ces écrans. Pendant les fondus,
transferts de ressources ou écritures réelles de sauvegarde EEPROM, le menu attend :
toutes les images d'une transition ne peuvent pas être interrompues.

## 5. Revoir l'introduction ou la fin

Pendant le jeu normal, choisissez **WORLD / WARP → STORY INTRO** ou **ENDING / CREDITS**.
Appuyez sur **A**, puis une seconde fois sur **A** pour confirmer. Ne lancez pas une
relecture pendant un dialogue, une cinématique ou depuis l'inventaire natif.

**RETURN FROM REPLAY** termine la relecture et recharge la salle d'origine. Les
1 204 octets de sauvegarde native sont conservés en RAM puis restaurés au retour.
Les acteurs temporaires de la salle ne sont pas restaurés comme avec un état instantané
d'émulateur. Les autres modifications, les codes de triche et le chronométrage sont
temporairement suspendus. La relecture de la fin revient avant la demande normale
de sauvegarde ; une fin normale conserve son fonctionnement d'origine.
Réinitialiser l'émulateur fait perdre le point de retour temporaire en RAM.

## 6. Outils d'acteurs

**ACTORS / OBJECTS → RAW NATIVE SPAWNER** expose les catégories et noms natifs ainsi
que les valeurs brutes kind/ID/type/type2/timer/subtimer/flags/parent/layer.
**QUICK SPAWN PRESETS** ne limite pas le mode brut. Il n'existe pas de liste restrictive
de salles/étages/variantes ; les routines natives absentes et les réserves d'acteurs
saturées sont toutefois refusées. 546 emplacements natifs et 118 types d'objets au sol
sont étiquetés ; 25 entrées restent volontairement **UNKNOWN**.

Dans **ROOM ACTOR LIST**, choisissez un acteur pour le geler, le supprimer ou le déplacer.
**LINK TO ACTOR** téléporte Link vers l'acteur ; **ACTOR TO LINK** amène l'acteur à Link,
dans la salle actuelle, sans changement de salle. Les gestionnaires sans structure XYZ
commune affichent **NO GENERIC XYZ** : le déplacement générique est indisponible,
mais le gel et la suppression restent possibles. Gelez d'abord un acteur si son IA le déplace à nouveau.

Un deuxième Link, des boss/types incompatibles ou des dépendances de salle/script/parent
manquantes peuvent faire planter ou bloquer le jeu. Supprimer un boss **ne compte pas**
comme une victoire. Si nécessaire, rechargez sans sauvegarder.

## 7. Enemy Freeze, Break Free et Infinite Time

- **Enemy Freeze :** inclut les boss et leurs parties visibles sans collision propre. Les projectiles d'autres catégories d'acteurs peuvent nécessiter un gel individuel.
- **Break Free :** ferme le texte actif via l'état de fermeture natif et rend le contrôle au joueur. Les scripts déjà exécutés ne sont pas annulés ; d'autres scripts peuvent reprendre le contrôle plus tard.
- **Infinite Time :** couvre le compte à rebours des poules d'Anju, celui du château d'Hyrule sombre, l'activation temporaire des interrupteurs en forme d'œil et la durée des amulettes/potions de chance déjà actives. Ce n'est pas un gel de tous les compteurs, animations ou cinématiques. Désactivez-le pour permettre l'évaluation des comptes à rebours, les récompenses et la fin des effets temporaires.

## 8. Tests, problèmes et contributions

Cette version a réussi **166 suites automatisées mGBA : 53 USA + 113 EU/JP**.
Le testeur du projet a aussi confirmé l'ouverture du menu et le No-Clip dans son
émulateur. Une validation Android complète pour chaque région n'a pas été documentée ;
aucun test sur GBA réelle n'a été effectué. Voir la [vérification](VERIFICATION.md).

Signalez les problèmes dans [Issues](https://github.com/Nimcoz/TMC-Practice/issues),
avec la région, la version du patch, l'émulateur et sa version, les étapes et, si utile,
une capture d'écran. N'envoyez aucune ROM, sauvegarde privée ou donnée de connexion.
Toute contribution au projet officiel nécessite l'accord préalable de Nimcoz ;
les signalements de bugs n'en ont pas besoin. Voir les [règles de contribution](../CONTRIBUTING.md)
et les [crédits et droits](../THIRD_PARTY_NOTICES.md).
