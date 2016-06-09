###Description

формат JSON предложений:

--------------
###JSONroot {"ESP_Sett":{objects}}

настройка модуля(objects):

####"hostIP":"string"
 - IP сервера например: "hostIP":"192.168.0.15" 
 
####"locIP":"string"
 - IP ESP8266 в STA режиме например: "locIP":"192.168.0.115" 
 
####"nSSID":"string"
 - SSID сети   например: "nSSID":"MyWiFi" 
 
####"nPass":"string"
 - Password сети   например: "nSSID":"MyPass"  

####"nPort":int
 - TCP порт сервера  например: "nPort":48500 
 
####"bRate0":int
 - Скорость UART модуля ESP8266  например: "bRate0":115200 
 
####"reset":1
 - Команда рестарта модуля ESP8266 
 
---------------------------------------------
###JSONroot {"anykey":anyObjects}

-любой JSON ключ отличный от "ESP_Sett" - прозрачно транспортируется UART <-> TCP
 
