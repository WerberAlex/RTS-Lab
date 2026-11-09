# SPDX-License-Identifier: Apache-2.0

*** Settings ***
Documentation     Test avancé du shell Zephyr (shell_module) exécuté dans Renode.
...               Chaque cas de test démarre une machine neuve et exerce les commandes
...               définies dans main.c, test_module.c, dynamic_cmd.c et uart_reinit.c.
Resource          ${KEYWORDS}
Test Setup        Boot Shell

*** Variables ***
${PROMPT}         uart:~$
${BOARD}          stm32f7

*** Keywords ***
Boot Shell
    [Documentation]    Charge la machine, démarre l'émulation et attend le prompt du shell.
    Prepare Machine
    Start Emulation
    Wait For Prompt On Uart    ${PROMPT}

Wait For Text
    [Documentation]    Attend une ligne contenant ${expected} (ou correspondant à l'expression
    ...                régulière si ${regex} vaut True).
    [Arguments]        ${expected}    ${regex}=False    ${timeout}=5
    Wait For Line On Uart      ${expected}    treatAsRegex=${regex}    timeout=${timeout}

Command Should Print
    [Documentation]    Envoie une commande puis attend la ligne attendue.
    [Arguments]        ${command}    ${expected}    ${regex}=False    ${timeout}=5
    Write Line To Uart         ${command}
    Wait For Text              ${expected}    ${regex}    ${timeout}

Send Control Character
    [Documentation]    Envoie un octet brut sur l'UART via le moniteur Renode. Robot ne peut pas
    ...                transmettre Ctrl-X / Ctrl-Q lui-même (caractères interdits en XML).
    [Arguments]        ${code}
    Execute Command            ${UART} WriteChar ${code}

*** Test Cases ***
Should Report Version Board And Ping
    [Documentation]    Commandes simples de main.c : version, demo board, demo ping.
    [Tags]             basic
    Command Should Print    version       Zephyr version \\d+\\.\\d+\\.\\d+    regex=True
    Command Should Print    demo board    ${BOARD}
    Command Should Print    demo ping     pong

Should Pass Arguments To Handlers
    [Documentation]    demo params : argc et argv, y compris un argument entre guillemets.
    [Tags]             arguments
    Write Line To Uart      demo params a b c
    Wait For Text           argc = 4
    Wait For Text           argv[0] = params
    Wait For Text           argv[1] = a
    Wait For Text           argv[2] = b
    Wait For Text           argv[3] = c
    Write Line To Uart      demo params "hello world" x
    Wait For Text           argc = 3
    Wait For Text           argv[1] = hello world
    Wait For Text           argv[2] = x

Should Hexdump Arguments
    [Documentation]    demo hexdump : octets en hexadécimal puis colonne ASCII.
    [Tags]             arguments
    Write Line To Uart      demo hexdump abc
    Wait For Text           argc = 2
    Wait For Text           00000000: 68 65 78 64 75 6d 70
    Wait For Text           00000000: 61 62 63
    Write Line To Uart      demo hexdump abc
    Wait For Text           \\|abc +\\|    regex=True

Should Resolve Every Dictionary Entry
    [Documentation]    demo dictionary : chaque nom de la table renvoie sa valeur.
    [Tags]             arguments
    FOR    ${i}    IN RANGE    4
        Command Should Print    demo dictionary value_${i}    (value_${i}, ${i})
    END

Should Manage Dynamic Commands
    [Documentation]    Cycle de vie complet de dynamic_cmd.c : ajout, tri alphabétique,
    ...                exécution, erreurs, suppression.
    [Tags]             dynamic
    Command Should Print    dynamic show              Please add some commands first.
    Command Should Print    dynamic add test1         command added successfully
    Command Should Print    dynamic add alpha         command added successfully
    Write Line To Uart      dynamic show
    Wait For Text           Dynamic command list:
    Wait For Text           \\[ +0\\] alpha    regex=True
    Wait For Text           \\[ +1\\] test1    regex=True
    Command Should Print    dynamic execute test1     dynamic command: test1
    Command Should Print    dynamic execute nope      execute: unknown parameter: nope
    Command Should Print    dynamic add bad-name      please use only alphanumerical characters
    Command Should Print    dynamic add test1         duplicated command
    Command Should Print    dynamic remove test1      command removed successfully
    Write Line To Uart      dynamic show
    Wait For Text           \\[ +0\\] alpha    regex=True
    Should Not Be On Uart   test1    timeout=1

Should Check Parameter Counts
    [Documentation]    dynamic add exige exactement 1 argument (SHELL_CMD_ARG avec 2 obligatoires,
    ...                nom de la commande compris) : le shell refuse 0 ou 2 arguments.
    ...                demo ping est déclarée avec SHELL_CMD (0 obligatoire, 0 optionnel) : un
    ...                argument en trop n'est pas refusé, la commande répond quand même.
    [Tags]             errors
    Command Should Print    dynamic add               wrong parameter count
    Command Should Print    dynamic add one two       wrong parameter count
    Command Should Print    demo ping extra           pong

Should Run Section Commands
    [Documentation]    section_cmd : sous-commandes enregistrées depuis deux fichiers
    ...                (main.c et test_module.c).
    [Tags]             sections
    Command Should Print    section_cmd cmd1              cmd1 executed
    Command Should Print    section_cmd cmd2              cmd2 executed
    Command Should Print    section_cmd cmd1 sub_cmd1     sub cmd1 executed

Should Log At Every Level While Timer Runs
    [Documentation]    log_test : le timer émet des logs info, warning et erreur toutes les
    ...                200 ms, puis s'arrête sur demande.
    [Tags]             logging
    Command Should Print    log_test start demo    Log test started
    Wait For Text           <inf> app: Timer expired.
    Wait For Text           <inf> app_test: info message
    Wait For Text           <wrn> app_test: warning message
    Wait For Text           <err> app_test: err message
    Wait For Text           <inf> app: Timer expired.
    Command Should Print    log_test stop          Log test stopped
    # Un aller-retour vide les logs encore en attente, puis on vérifie qu'aucun tick ne revient.
    Command Should Print    demo ping              pong
    Should Not Be On Uart   Timer expired          timeout=1

Should Capture Raw Input In Bypass Mode
    [Documentation]    bypass : les octets reçus sont affichés en hexadécimal jusqu'à
    ...                Ctrl-X Ctrl-Q, puis le shell redevient interactif.
    [Tags]             bypass
    Command Should Print    bypass    Bypass started
    Write Char On Uart      A
    Wait For Text           41.*\\|.*A    regex=True
    Send Control Character  0x18
    Send Control Character  0x11
    Wait For Text           Exit bypass
    Command Should Print    demo ping    pong

Should Release And Restore The Shell UART
    [Documentation]    shell_uart_release : l'UART passe en écho brut, puis la touche x
    ...                réinitialise le shell.
    [Tags]             uart
    Command Should Print    shell_uart_release    Uninitializing shell
    Write Char On Uart      h
    Write Char On Uart      i
    Write Char On Uart      x
    Wait For Text           hix
    Wait For Prompt On Uart    ${PROMPT}    timeout=3
    Command Should Print    demo ping    pong
