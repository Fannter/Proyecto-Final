El codigo presentado en este repositorio es valido unicamente utilizando el siguiente circuito PCB. El mismo fue diseñado en Proteus 8.

Diseño del PCB:
![image](https://github.com/Fannter/Proyecto-Final/assets/100977206/098e4124-f3e4-4c5f-950a-ac5fa57bf4f0)

Esquema electrónico:
![image](https://github.com/Fannter/Proyecto-Final/assets/100977206/f78ded8c-e603-4f34-82ef-59e990e32041)

Vista en 3D del mismo:
![image](https://github.com/Fannter/Proyecto-Final/assets/100977206/448d43dc-c2bb-4873-977b-74e9b3624c08)

Utilizando los siguientes pines en Arduino UNO:

|Pin  |  Componente  |  Descripción                    |
------|--------------|---------------------------------|
|D2   |  Transistor  |  Detector Cruce por cero        |
|D3   |  Triac       |  Resistencia Calefactora        |
|D4   |  Encoder     |  Pulsador Encoder               |
|D6   |  Encoder     |  Pin B de encoder con pulsador  |
|D7   |  Encoder     |  Pin A de encoder con pulsador  |
|D9   |  DHT11       |  Sensor Humedad                 |
|D10  |  J9          |  Motor 220V                     |
|D11  |  J7          |  Ventilador                     |

//////////////////////////////////////////////////////////

|A0|LM35|Sensor Temperatura|
|--|----|---------------|
|A4|LCD+I2C|Pin SDA de LCD con módulo I2C|
|A5|LCD+I2C|Pin SCL de LCD con módulo I2C|


