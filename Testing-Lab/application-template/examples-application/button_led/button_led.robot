# SPDX-License-Identifier: Apache-2.0

*** Settings ***
Documentation     Test d'intégration : firmware complet sur stm32f7 dans Renode.
...               Le bouton B1 (PI11) est ajouté à la plateforme ; la LED LD1 (PI1) existe déjà.
Resource          ${KEYWORDS}
Test Setup        Boot With Button

*** Variables ***
${BUTTON}         sysbus.gpioi.user_button
${LED}            sysbus.gpioi.greenled1

*** Keywords ***
Boot With Button
    Prepare Machine
    Execute Command       machine LoadPlatformDescriptionFromString "user_button: Miscellaneous.Button @ gpioi 11 { -> gpioi@11 }"
    Create LED Tester     ${LED}    defaultTimeout=1
    Start Emulation
    Wait For Line On Uart    LED spec flags: 0x0
    Wait For Line On Uart    Button spec flags: 0x0

*** Test Cases ***
LED Should Be Off While Button Is Released
    Wait For Line On Uart    Button state: 0
    Assert And Hold LED State    false    timeoutAssert=1    timeoutHold=0.5

LED Should Follow Button
    Execute Command          ${BUTTON} Press
    Wait For Line On Uart    Button state: 1
    Assert LED State         true
    Execute Command          ${BUTTON} Release
    Wait For Line On Uart    Button state: 0
    Assert LED State         false
