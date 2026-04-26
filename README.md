# Documentação

Prototipo de ar condicionado com IoT e Esp32

## Arquitetura/componentes

### 1. Funcionalidades

Mapeamento de funções que realiza:

- **Liga/Desliga remotamente**: Salvar o estado anterior, ligar ao
    se aproximar (geolocalização), caso desejado.
- **Rotinas automaticas**: Realizar uma ação caso algo aconteça,
    ex.: Caso a temperatura seja maior que 22º ligue o aparelho
- **Temperatura**: De 16º a 30º celsius(possível mudar)
- **Velocidades**: Split 20% até 100% da velocidade (possível mudar)
- **Swing**: Fixada em algum lugar ou mexendo o tempo todo
- **Umidificador**: Controlar a umidade local
- **Modo sleep**: Deixar programado como irá se comportar durante 12h
- **Time On/Off**: Deixar programado quanto tempo ficará ligado
- **Emitir cheiros**: Escolher perfumes para aromatizar o ambiente
- **Recirculação do ar**: Apenas movimentar o ar sem demais alterações
- **Integração com ambiente**: Desligamento automatico caso a janela seja aberta, comece a chover, etc. (Janela com
    Iot, clima ...)
- **Salvar estado**: Em queda de energia salva o ultimo estado e 
    aplica novamente quando retornar a energia
- **Monitoramento de dados**: Registrar todas as funcionalidades anteriores
    mais a qualidade do ar (Poluição), consumo de energia, tempo de uso, 
    historico de temperatura, etc.
- **Controle de acesso**: Poder escolher quem vai mexer no ar condicionado

### 2. Meios de controle
- **Dispositivos**: Celulares, Tablets, SmartWatchs, Laptops, PCs 
    que tenham Wi-Fi para realizar a comunicação a distancia e telemetria.


