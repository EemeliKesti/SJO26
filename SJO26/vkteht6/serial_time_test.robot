*** Settings ***
Library   String
Library   SerialLibrary

*** Variables ***
${com}      COM5
${baud}     115200
${board}    nRF5340

*** Test Cases ***
Connect Serial
	Log To Console  Connecting to ${com} ${board}
	Add Port  ${com}  baudrate=${baud}  encoding=ascii  timeout=5
	Port Should Be Open  ${com}
	Reset Input Buffer
	Reset Output Buffer

# --- Oikeat aikamerkkijonot: odotetaan sekuntien määrä ---
Valid 1 min 20 s
	Time String Should Return  000120  80

Valid 59 s
	Time String Should Return  000059  59

Valid 10 min
	Time String Should Return  001000  600

Valid 1 h
	Time String Should Return  010000  3600

Valid max 23 59 59
	Time String Should Return  235959  86399

# --- Virheelliset aikamerkkijonot: odotetaan virhekoodi ---
# -1 = väärä pituus, -3 = virheellinen arvo
Invalid seconds 67
	Time String Should Return  001067  -3

Invalid minutes 60
	Time String Should Return  006000  -3

Invalid hours 24
	Time String Should Return  240000  -3

Invalid non-digit character
	Time String Should Return  00a120  -3

Invalid negative sign
	Time String Should Return  -00120  -3

Invalid too short
	Time String Should Return  00012  -1

Invalid empty string
	Time String Should Return  ${EMPTY}  -1

Disconnect Serial
	Log To Console  Disconnecting ${board}
	[TearDown]  Delete Port  ${com}

*** Keywords ***
Time String Should Return
	[Arguments]  ${time_str}  ${expected}
	# lähetetään aikamerkkijono + lopetusmerkki X
	Write Data  ${time_str}X  encoding=ascii
	Log To Console  Sent ${time_str}X
	# luetaan vastaus lopetusmerkkiin X (58) asti
	${read} =  Read Until  terminator=58  encoding=ascii
	Log To Console  Received ${read}
	# poistetaan lopetusmerkki ja verrataan lukuna
	${value} =  Replace String  ${read}  X  ${EMPTY}
	Should Be Equal As Integers  ${value}  ${expected}
