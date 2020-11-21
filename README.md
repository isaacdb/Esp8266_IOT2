# Exercicio Esp8266
<p align="center">Exercicio para aula de IOT2</p>

<p align="center">
 <a href="#-sobre-o-projeto">Sobre o Projeto</a> •
 <a href="#funcionalidades">Funcionalidades</a> • 
 <a href="#materiais-utilizados">Materiais Utilizados</a> • 
 <a href="#tecnologias">Tecnologias</a> • 
 <a href="#como-executar">Como Executar</a> • 
 <a href="#autor">Autor</a>
</p>

## 🎨 Layout
<div align="center">
<img width="80%" alt="FotoCircuito" title="#FotoCircuito" src="https://github.com/isaacdb/Esp8266_IOT2/blob/main/Screenshots/foto.jpeg">
</div>

---
## 💻 Sobre o projeto
<p>Com a placa Wemos32 D1, executamos uma aplicação que acessará um servidor remoto criado separadamente.</p>
<p>A aplicação acessará a rede wifi que estiver vinculada ao servidor, e após a conexão com o wifi, se conectará ao servidor. Com as conexões estabelecidas, será possivel ao usuário enviar determinadas mensagem a placa wemos, que então, fará a leitura da mensagem, e retornará ao servidor a leitura de um sensor correspondente a mensagem.</p>

---
## Materiais Utilizados

- Placa Wemos D1
- TCP Terminal APP
- Smartphone
- Sensor Ultrassônico HC-SR04
- Sensor Temperatura e Humidade DHT
- PushButton
- Led
- Resistores 150Ω
- Jumpers <s>(de preferencia funcinando)</s>

---

---
## Funcionalidades

- [x] Conexão Wifi monitorada
- [x] Conexão servidor via socket
- [x] Leitura de sensor por dado recebido

---
### Tecnologias

As seguintes tecnologias foram usadas na construção do projeto:

- [C]
- [FreeRTOS]

---
# Como executar
<h4>Pré requisitos</h4>
<p>Primeiramente faça a instalação dos arquivos para o uso do Esp8266, é indicado seguir um tutorial muito bem avaliado e estruturado como este <a href="https://www.youtube.com/watch?v=84tuQaV8N0g&feature=youtu.be">Professor Vagner</a></p>

<p>Faça o download do projeto via <a href="https://github.com/isaacdb/Esp8266_IOT2">GitHub/isaacdb</a></p>

<p>Faça o download do programa <a href="https://play.google.com/store/apps/details?id=com.hardcodedjoy.tcpterminal&hl=pt_BR&gl=US">TCP Terminal</a> em seu smartphone.</p>
<p>Configure-o para estar preparado para a conexão da placa Wemos.</p>
<div><img width="60%" alt="ConfigServidor" title="#ConfigServidor" src="https://github.com/isaacdb/Esp8266_IOT2/blob/main/Screenshots/servidor.jpeg"></div>

<h4>Circuito</h4>
<p>Monte o prototipo seguindo o circuito a seguir.</p>
<p>Confira se as pinagens estão correspondentes as do código, e cheque se todos os jumpers estão bem conectados.</p>
<img width="80%" alt="TinkerCad" title="#TinkerCad" src="https://github.com/isaacdb/Esp8266_IOT2/blob/main/Screenshots/circuito.PNG">

<h4>Configurações</h4>
<p>Abra o prompt de comando mingw32.exe</p>
<p>Navegue ate a pasta de donwload do projeto, a pasta que contém o arquivo Makefile, com o comando cd "caminho para a pasta"</p>
<p>EX:</p>

 > cd "C:\esp8266\ESP8266_RTOS_SDK\examples\IoTII\Exercicio_4\Esp8266_IOT2"
 
<p>Insira o comando "Make Menuconfig" para abrir o terminal de configuração, nele voce irá configurar tudo que é necessário para a aplicação funcionar corretamente.</p>
<img width="80%" alt="MenuConfig" title="#MenuConfig" src="https://github.com/isaacdb/Esp8266_IOT2/blob/main/Screenshots/menuconfig.PNG">
 <p>Acesse Serial Flasher config, e defina a porta correta que sua placa wemos esta vinculada</p>
 
 <img width="80%" alt="SerialPort" title="#SerialPort" src="https://github.com/isaacdb/Esp8266_IOT2/blob/main/Screenshots/serialport.PNG">
 <p>Para verificar qual a porta de conexão de sua placa acesse o gerenciador de dispositivos.</p>
   <img width="49%" alt="SerialPort" title="#SerialPort" src="https://github.com/isaacdb/Esp8266_IOT2/blob/main/Screenshots/gerenciadordedispositivos.PNG"> 
 
<p>Agora passe as configurações(Ip e port) do seu servidor criado pelo TCP Terminal para a placa, acesse a opção Example Configuration.</p>
   <img width="80%" alt="ConfigServ" title="#ConfigServ" src="https://github.com/isaacdb/Esp8266_IOT2/blob/main/Screenshots/conexaoservidor.PNG"> 
   
<p>Em seguida acesse a opção Example Connection Configuration, e insira os dados da rede wifi que será roteada pelo seu celular</p>

   <img width="80%" alt="ConnectWifi" title="#ConnectWifi" src="https://github.com/isaacdb/Esp8266_IOT2/blob/main/Screenshots/conexaowifi.PNG"> 
   
<h4>Execução</h4>
<p>Agora com todos os parametros configurados, no prompt de comando, insira o comando "make flash monitor", será inicializada a compilação do código para a placa wemos.</p>
<p>Se tudo ocorrer como o esperado, você vizualizará uma tela semelhante a está.</p>
   <img width="80%" alt="Conectado" title="#Conectado"   
   src="https://github.com/isaacdb/Esp8266_IOT2/blob/main/Screenshots/conectado.PNG"> 
   
<p>Com tudo em funcionamento insira alguma mensagem no TCP Terminal, então o wemos que esta conectado ao servidor, receberá a mensagem e responderá de acordo.</p>

   <img width="49%" alt="MSGServidor" title="#MSGServidor"   
   src="https://github.com/isaacdb/Esp8266_IOT2/blob/main/Screenshots/msgservidor.jpeg"> 
   
<p>Acabamos de aplicar alguns conceitos muito importantes para a area de IOT, a partir disso, tendo essa base, podemos expandir e criar outras inúmeras aplicações e automações.</p>

---
### Autor
Isaac Debiasi
