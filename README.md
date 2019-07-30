# Hidro
Software utilizado en el Proyecto Hidro

Consta básicamente de 2 programas:

*HidroA: para ser cargado en Arduino Nano,
dada la cantidad de entradas y salidas requeridas,
dicho software es el encargado de controlar la plantación.

*HidroN: (opcional) para ser cargado en NodeMCU,
este programa se encarga de solicitar datos de HidroA y los transmite a la nube,
así como también poder recibir datos de control de la nube y enviarlos a HidroA.
En este caso utilizamos el servicio de Cayenne MQTT.
