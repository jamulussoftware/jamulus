( [en](https://github.com/corrados/jamulus/blob/master/src/res/homepage/manual.md) | es )

Ayuda para Jamulus (Manual del Software)
==============================

Ventana Principal
-----------

![Main window](main.png)

### LEDs de Estado

![LEDs](led.png)

El LED de **Estado de Retardo** muestra el estado actual del retardo de audio:

* **Verde** - El retardo es perfecto para una jam session

* **Amarillo** - Una sesión aún es posible pero quizá sea más difícil tocar

* **Rojo** - El retardo es demasiado grande para tocar

**Buffers** muestra el estado actual del flujo de audio. Si está **rojo**, hay interrupciones en el flujo de audio. Esto puede ser causado por alguno de los siguientes problemas:

- El jitter buffer de red no es lo suficientemente grande para el jitter actual de la red/interfaz de audio.
- El retardo de buffer de la tarjeta de audio (tamaño buffer) tiene un valor demasiado bajo (ver ventana de Configuración).
- La tasa de subida o bajada is demasiado alta para tu ancho de banda de internet.
- El procesador del cliente o del servidor está al 100%.

### Nivel de entrada

![Input level](inputlevel.png)

Esto muestra los niveles de los dos canales estéreo de tu entrada de audio.
Asegúrate de no clipear la señal de entrada para evitar distorsiones de la señal de audio (los LEDs indicarán el clipeo cuando suceda).



### Botón de chat

![Chat dialog](chat.png)

El texto escrito en la ventana del chat es enviado a todos los clientes conectados. Si llega un nuevo mensaje de chat y la venta no está abierta, se abrirá automáticamente para todos los clientes.

### Ventana de perfil

![My profile dialog](profile.png)

Desde el menú de Ver, selecciona Mi Perfil... para establecer tu Alias/Nombre, mostrado debajo de tu fader en la ventana principal del cliente de Jamulus. Si se establece un instrumento y/o un país, los iconos de estas selecciones también se mostrarán bajo tu fader. El nivel de habilidad cambia el color de fondo de la etiqueta del fader y la ciudad se muestra en el tooltip de la etiqueta (ver pantallazo abajo).

![Fader tag tool tip](fadertagtooltip.png)

### Botón Conexión/Desconexión

Abre una ventana donde puedes seleccionar un servidor. Si estás conectado, pulsar este botón finalizará la sesión.

![Connect dialog](connect.png)

La ventana de Configuración de Conexión muestra una lista de servidores disponibles junto con el número de usuarios conectados y el número máximo de los mismos soportado. Los operadores de servidores pueden, opcionalmente, listar su servidor por género musical. Utiliza el menú desplegable de Lista para elegir un género, haz clic sobre el servidor al que te quieres unir y otra vez sobre el botón de Conectar para conectarte. También puedes hacer doble clic sobre el nombre del servidor. Los servidores permanentes (aquellos que llevan más de 24 horas en la lista) se muestran en negrita.

Puedes filtrar la lista por nombre de servidor o ubicación. Para solo mostrar los servidores ocupados, escribe el caracter "#".

Si conoces la dirección IP o URL de un servidor, puedes conectarte a él utilizando el campo de Nombre/Dirección del Servidor. Se puede añadir un número de puerto opcional tras la dirección IP o URL utilizando dos puntos como separador, por ej.
jamulus.dyndns.org:22124. El campo también mostrará una lista de los servidores utilizados recientemente.

### Botón de Silenciarme Yo

Corta tu flujo de audio al servidor de modo que te escucharás y verás tus propios niveles de audio, pero otros músicos no. Ten en cuenta que otros músicos no sabrán si te has silenciado.

### Efecto Reverb

![Reverberation](reverberation.png)

Se puede aplicar un efecto de reverberación a un canal local de audio mono o a ambos canales en modo estéreo. Se puede modificar la selección de canales en modo mono y el nivel de reverberación. Por ejemplo, si la señal del micrófono va por el canal derecho de la tarjeta de audio y se desea aplicar reverberación, cambia el selector de canal a derecho y sube el fader hasta alcanzar el nivel de reverberación deseado.

### Control local de paneo / balance

Control local de paneo / balance

Controla los niveles relativos de los canales locales de audio derecho e izquierdo. Para una señal mono actúa como paneo entre los dos canales. Por ejemplo, si se conecta un miocrófono al canal derecho y un instrumento al izquierdo que suena mucho más alto que el micrófono, mueve el fader en una dirección donde la etiqueta sobre el fader muestra L -x, donde x es el indicador de atenuación actual.

### Mezclador de audio del servidor

![Audio faders](faders.png)

En la ventana del mezclador de audio, se muestra un fader para cada cliente conectado al servidor (incluyéndote a ti).
Los faders te permiten ajustar el nivel de lo que escuchas sin afectar a lo que escuchan los demás.

El vúmetro muestra el nivel de entrada en el servidor - esto es, el sonido enviado.

Si has configurado tu Canal de Audio a Estéreo o Salida Estéreo en tu Configuración, también verás un control de paneo.

Si ves un icono de "silenciado" sobre un canal, significa que ese músico no puede escucharte. Puede que te hayan silenciado, aplicado 'solo' a uno o más canales sin incluir el tuyo, o han bajado tu fader en su mezcla a cero.

Utilizar el **botón de Mute** evita que el canal indicado se escuche en tu mezcla local. Ten en cuenta que cuando muteas a un músico, verán un icono de "silenciado" sobre tu fader para indicar que no puedes escucharle. Ten en cuenta también que continuarás viendo mover su vúmetro si el sonido del músico muteado llega al servidor. La posición del fader para ellos en tu mezcla no se verá afectada.

El **botón de Solo** te permite escuchar uno o más músicos asilados de los demás. Los que no lo tengan aplicado estarán silenciados. Ten en cuenta que aquellos músicos que no lo tienen aplicado verán el icono de "silenciado" sobre tu fader.

Los canales se muestran de izquierda a derecha en el orden en el que se conectan los clientes hasta que se marchan, momento en el cual su "posición" es ocupada por cada nueva conexión. Puedes cambiar el orden utilizando la opción Editar en el menú de la aplicación.

Puedes agrupar faders utilizando el botón de "grupo" (GRP). Si mueves el fader de cualquier miembro del grupo, los demás del grupo también se moverán en la misma proporción.

Si el operador del servidor ha habilitado la grabación, verás un mensaje sobre el mezclador indicando que estás siendo grabado.



Ventana de Configuración
---------------

![Settings](settings.png)

### Dispositivo de Audio

![Sound card device Windows](soundcarddevicewindows.png)
![Sound card device Mac](soundcarddevicemac.png)

El driver ASIO (tarjeta de audio) se puede seleccionar utilizando Jamulus en el sistema operativo Windows. Si el driver ASIO no es válido se mostrará un mensaje de error y el anterior driver válido será seleccionado. En Mac, se puede seleccionar el hardware de entrada y salida.

### Mapeo canales entrada/salida

![Channel mapping](channelmapping.png)

Si el dispositivo de audio ofrece más de un canal de entrada o salida, son visibles las configuraciones para el _Mapeo de Canales de Entrada y de Salida_. Para cada canal de entrada/salida (canal Izquierdo y Derecho) de Jamulus se puede seleccionar un canal diferente de la tarjeta de audio.

### Activar Buffers de Red Pequeños

Permite soporte para paquetes de audio por red muy reducidos. Solo se utilizan si el retardo del buffer de la tarjeta de audio es menor de 128 muestras. Cuanto menores los buffers de red, menor la latencia de audio. Pero al mismo tiempo, aumenta la carga de red y la probabilidad de caídas de audio también aumenta.

### Retardo de Buffer

![Buffer delay](bufferdelay.png)

El retardo de buffer es un parámetro fundamental en Jamulus. Este parámetro tiene un impacto sobre muchas propiedades de la conexión. Hay soporte para tres tamaños de buffer:

- 64 muestras: Es la configuración aconsejada puesto que ofrece la latencia más baja, aunque no funciona con todas las tarjetas de audio.
- 128 muestras: Debería de funcionar con la mayoría de tarjetas de audio.
- 256 samples: Solo debería usarse con un ordenador muy lento o con una conexión a internet muy lenta.

Algunos drivers de tarjetas de audio no permiten cambiar el retardo de buffer desde dentro de Jamulus. En este caso la configuración del retardo de buffer se deshabilita y hay que cambiarlo utilizando el driver de la tarjeta de audio. En Windows, pulsa el botón de Configuración ASIO para acceder al panel de configuración.

![Buffer delay Windows](bufferdelaywindows.png)

En Linux, utiliza la herramienta de configuración de Jack para cambiar el tamaño del buffer.

El retardo del buffer tiene un impacto en el estado de la conexión, la tasa de subida y el retardo total. Cuanto menor sea el buffer, mayor la probabilidad de que el indicador de estado esté en rojo (caídas de audio), mayor la tasa de subida y menor el retardo total.

![Buffer delay dependencies](bufferdelaydependency.png)

Por tanto la configuración del buffer es un compromiso entre calidad de audio y retardo total.

### Jitter buffer con indicador del estado del buffer

![Jitter buffer](jitterbuffer.png)

El jitter buffer compensa el jitter de la red y la tarjeta de audio. El tamaño de este buffer tiene por tanto un impacto sobre la calidad del flujo de audio (el número de caídas de la señal) y el retardo total (a mayor buffer, mayor retardo).

El tamaño del jitter buffer se puede establecer manualmente para el cliente local y para el servidor remoto. Para el jitter buffer local, las caídas del flujo de audio se indican mediante la luz debajo de los faders del jitter buffer. Si la luz se vuelve roja, significa que ha habido una interrupción del flujo de audio.

Por tanto la configuración del jitter buffer es un compromiso entre calidad y retardo total.

Si se activa la casilla de Auto, los jitter buffers del cliente local y del servidor remoto se configuran automáticamente basándose en mediciones del jitter de la red y la tarjeta de audio. Si se activa esta opción, los faders quedan deshabilitados (no pueden moverse con el ratón).

### Canales de Audio

![Audio channels](audiochannels.png)

Selecciona el número de canales de audio a utilizar para la comunicación entre cliente y servidor. Hay tres modos disponibles:

Los modos Mono y Estéreo utilizan uno y dos canales de audio respectivamente.

Entrada-mono/Salida-estéreo: La señal de audio enviada al servidor es mono pero la señal que vuelve es estéreo. Esto es útil si la tarjeta de audio tiene un instrumento en un canal de entrada y un micrófono en el otro. En ese caso las dos señales de entrada pueden combinarse en un canal mono pero la mezcla del servidor se escucha en estéreo.

Activar el modo estéreo aumentará la tasa de envío de datos. Asegúrate de que la tasa de subida no excede el ancho de banda disponible en tu conexión a internet.

En el caso del modo estéreo, no estará disponible la selección de canal para el efecto de reverb en la ventana principal puesto que en este caso el efecto se aplicará a ambos canales.

### Calidad del Audio

![Audio quality](audioquality.png)

Cuanto mayor la calidad del audio, mayor la tasa de subida del audio. Asegúrate de que tu tasa de subida no excede el ancho de banda de tu conexión a Internet.

### Nivel cliente nuevo

![New client level](newclientlevel.png)

Este ajuste define el nivel del fader de una nueva conexión de cliente, en porcentaje. Si se conecta un nuevo cliente al servidor actual, el nivel inicial de su fader tomará este valor si no se ha especificado anteriormente un valor para ese cliente de una conexión anterior.

### Interfaz oscura

![Fancy skin](fancyskin.png)

Esto aplicará un aspecto oscuro a la ventana principal.

### Dirección personalizada del servidor central

![Central server address](centralserveraddress.png)

Deja esto en blanco a menos que necesites escribir la dirección de un servidor distinto a los que hay por defecto.

### Parámetro de estado de conexión actual

![Indicators](indicators.png)

El Ping es el tiempo que requiere el flujo de audio para viajar desde el cliente al servidor y volver. Este retardo lo determina la red y debería ser de unos 20-30 ms. Si este retardo es mayor de unos 50 ms, la distancia al servidor es demasiado grande o tu conexión a internet no es óptima.

El retardo total se calcula con el ping y el retardo ocasionado por la configuración de buffers.

La Tasa de Subida de Audio depende del tamaño actual de los paquetes de audio y la configuración de compresión de audio. Asegúrate de que la tasa de subida no es mayor que la velocidad de subida disponible (comprueba la tasa de subida de tu conexión a internet, por ejemplo con [speedtest.net](http://speedtest.net)).
