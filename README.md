# PSI-Microcontroladores2-Aula06
Atividade: Resolução de Race Condition com Semáforo

--------------------------------------------------------

## Integrantes:
- Arthur Junqueira C B
- Gustavo Fernandes
- Rafael dos Reis

--------------------------------------------------------

## Cenário escolhido:
Race condition na manipulação de uma variável global

--------------------------------------------------------

## Revisão do código:
### Código do ARTHUR (avaliado por Gustavo): 
- [Link Código fonte com erros]:
- 		https://github.com/Poli-USP-Microcontroladores2/semaforo-race-condition-tecnicos-da-enel/blob/arthur/codigoORIGINAL.c
- Comportamento incorreto:
- 		Foi observado um problema clássico de Lost Update: "Uma segunda transação escreve um segundo valor de um item de dados (dado) sobre o primeiro valor escrito por uma primeira transação concorrente, e o primeiro valor é perdido para outras transações em execução concorrente que precisam, por precedência, ler o primeiro valor. As transações que leram o valor errado terminam com resultados incorretos." fonte: https://en.wikipedia.org/wiki/Concurrency_control. Portanto, no nosso contexto, uma thread lê um valor da variável, é interrompida por um k_sleep, a próxima thread lê o mesmo valor, terminando com ambas imprimindo um valor repetido. 
- Momento do erro:
- 		O erro ocorre no intervalo de tempo entre a leitura e a escrita. O comando k_sleep permite que a segunda thread leia o valor da variável jantar antes que a primeira thread tenha terminado de atualizá-la.

### Código do GUSTAVO (avaliado por Rafael):
- [Link Código fonte com erros]:
- 		https://github.com/Poli-USP-Microcontroladores2/semaforo-race-condition-tecnicos-da-enel/blob/gustavo/src/c%C3%B3digoOriginal.c
- Comportamento incorreto:
- 		Pelos LOGs Apresentados pelo sistema, o valor esperado, que era de 1.000.000 nunca era alcançado, sempre ficando aleatorizado entre valores maiores que 100.000 e menores que 1.000.000, isso ocorre por conta da race condition que ocorre nas linhas 54 e 75, onde as threads A e B tentam manipular o contador ao mesmo tempo e ocorre que sobrescrevem o valor aficionado pela outra thread, dessa forma, resultando em um saldo final menor que o o saldo esperado.
# - Momento do erro:
		Na hora de incrementar o valor, caso ambas as threads tentem fazer ao mesmo tempo. O erro acontece durante a sequência de Leitura-Modificação-Escrita (Read-Modify-Write) não atômica dentro das threads A e B.

### Código do RAFAEL (avaliado por Arthur):
- [Link Código fonte com erros]:
- 		https://github.com/Poli-USP-Microcontroladores2/semaforo-race-condition-tecnicos-da-enel/blob/Rafael/codigoORIGINAL.c
- Comportamento incorreto:
- 		Ocorre uma "Sobrescrita de Dados" (Lost Update) devido a uma Condição de Corrida (Race Condition). A função main lê os dados do sensor para uma variável temporária, processa essa cópia, e depois salva o resultado de volta na variável global. O comportamento incorreto é que, enquanto a main estava ocupada processando a cópia antiga, a interrupção (ISR) atualizou a variável global com dados novos e reais do hardware. Quando a main finalmente escreve o seu resultado, ela ignora e apaga a atualização feita pela ISR, fazendo com que o sistema perca dados recentes e o estado do sensor "volte no tempo" (inconsistência). Exemplo do log: A ISR atualizou o valor para 250, mas a Main (que tinha lido 200 antes) calculou 230 e salvou 230 por cima, perdendo a leitura de 250.
- Momento do erro:
- 		O erro acontece durante a sequência de Leitura-Modificação-Escrita (Read-Modify-Write) não atômica dentro da função processamento_sensor_vulneravel (ou na demonstracao_corrupcao_explicita). Especificamente, o problema se manifesta no intervalo de tempo entre: A Leitura: memcpy(&temp, &sensor_data, ...) (Linha 55 e 116); A Escrita: sensor_data.valor = novo_valor; (Linha 73 e 126). Durante os atrasos (k_busy_wait) que existem entre essas duas linhas, a ISR é disparada e altera a memória global. Como a main não bloqueou o acesso (usando semáforos, mutex ou desabilitando interrupções), ela opera com uma "foto" velha da memória e, ao salvar, descarta o trabalho da ISR.

--------------------------------------------------------

## Casos de teste:
### Código do ARTHUR (feito por Arthur): 
- Pré-condição:
- 		Serial monitor com Baud Rate de 115200, arquivo platformio.ini inserido no projeto, arquivo prj.conf inserido no projeto, conteúdo do arquivo códigoORIGINAL.c colado dentro do arquivo main.c do projeto 
- Etapas de teste:
- 		Compilar e fazer upload do código para uma placa FRDM-KL25Z
- Pós-condição esperada:
- 		De vez em quando, o valor da variável "jantar" repete números, o que significa que ocorreu race condition

### Código do GUSTAVO (feito por Gustavo):
- Pré-condição:
  Placa FRDM-KL25Z conectada ao computador, com o código gravado na placa. Serial Monitor aberto em 115200 de baud rate, exibindo: 
<inf> race_condition_demo: === Demonstracao de RACE CONDITION ===
<inf> race_condition_demo: Pressione o botao (SW0) para iniciar o teste.
<inf> race_condition_demo: Valor esperado: 1000000
- Etapas de teste:
  Pressionar o botão SW0 (PTA16), observar os leds (verde e vermelho indicam a atividade das threads), aguardar a mensagem <inf> race_condition_demo: --- Teste Concluido --, e comparar o valor esperado com o valor real exibido.
- Pós-condição esperada:
  Deve ser exibido no serial monitor: <inf> race_condition_demo: Valor Real: 1000000

### Código do RAFAEL (feito por Rafael):
- Pré-condição: 
	Serial monitor com Baud Rate de 11500, arquivo platformio.ini inserido no projeto, arquivo prj.conf inserido no projeto, conteúdo do arquivo códigoORIGINAL.c colado dentro do arquivo main.c do projeto 
- Etapas de teste: 
	Compilar e fazer upload do código para uma placa FRDM-KL25Z
- Pós-condição esperada: 
  O timestamp regride de vez em quando, o que indica a ISR sobrescrita

--------------------------------------------------------

## Solução:
### Código do ARTHUR (feito por Arthur): 
- [Link Código fonte sem erros]: 
https://github.com/Poli-USP-Microcontroladores2/semaforo-race-condition-tecnicos-da-enel/blob/arthur/codigoCORRIGIDO.c
- Mudanças feitas:
	-- Criação do Semáforo: Foi declarada uma estrutura global struct k_sem semaforo_jantar.
	-- Inicialização: No main, o semáforo foi inicializado com k_sem_init(&semaforo_jantar, 1, 1). O valor inicial 1 indica que o recurso está disponível, e o limite 1 faz com que ele atue como um semáforo binário (comportamento de Mutex), garantindo que apenas uma thread entre por vez.
	-- Proteção de Seção Crítica (Writers): Nas funções chefe_chloe e chefe_david, adicionei k_sem_take antes de lerem a variável jantar e k_sem_give somente após escreverem o novo valor. Isso encapsula o comando k_sleep que causava a condição de corrida. Agora, se Chloe dormir segurando o semáforo, David será bloqueado ao tentar dar take, impedindo que ele leia o valor desatualizado.
	-- Proteção de Leitura (Reader): Adicionei o par take/give também no verificador_jantar para garantir que ele não leia o valor no meio de uma transição (embora o erro crítico fosse nos escritores).
- Resultado após correção:
	-- A "Race Condition" (Condição de Corrida) foi eliminada. Não haverá mais "Lost Updates" (atualizações perdidas).
	-- Antes: Chloe lia 0, dormia; David lia 0, dormia; Chloe escrevia 1; David escrevia 1. O jantar ficava em 1 (erro).
	-- Agora: Chloe pega o semáforo, lê 0 e dorme. David tenta pegar o semáforo e bloqueia (entra em estado de espera). Chloe acorda, escreve 1 e libera o semáforo. Só então David desbloqueia, lê 1 (o valor correto atualizado), soma 1 e escreve 2.
	-- Log esperado: A sequência numérica do jantar será sempre crescente e contínua (1, 2, 3, 4...), sem repetições de números consecutivos.
- [Link para a print do resultado]: 
https://github.com/Poli-USP-Microcontroladores2/semaforo-race-condition-tecnicos-da-enel/blob/arthur/CONFIRMA%C3%87%C3%83O%20QUE%20DEU%20CERTO.png

### Código do GUSTAVO (feito por Gustavo):
- [Link Código fonte sem erros]:
https://github.com/Poli-USP-Microcontroladores2/semaforo-race-condition-tecnicos-da-enel/blob/gustavo/src/c%C3%B3digoCorrigido.c
- Mudanças feitas:
    Implementação de Mutex para proteção da variável global que está sendo incrementada. Aplicação de "controle de fluxo" com uma flag de estado para lidar com ruídos, ou bouncing, do botão. Reset de variáveis feito na Main antes de liberar threads. Redução da quantidade de incremento por thread para 100000, para garantir uma velocidade maior nos testes.
- Resultado após correção:
    Race condition corrigida, mas o processamento está mais lento. Resultado no serial monitor: <inf> race_fixed_robust: SUCESSO: 200000
- [Link para a print do resultado]:
https://github.com/Poli-USP-Microcontroladores2/semaforo-race-condition-tecnicos-da-enel/blob/gustavo/print.png

### Código do RAFAEL (feito por Rafael):
- [Link Código fonte sem erros]: 
https://github.com/Poli-USP-Microcontroladores2/semaforo-race-condition-tecnicos-da-enel/blob/Rafael/codigocorrigido.c
-Mudanças feitas:
  Proteção com irq_lock()/irq_unlock(): Foi implementado o uso de irq_lock() e irq_unlock() para proteger as seções críticas onde os dados do sensor são acessados.
  Inicialização: Não foi necessária inicialização adicional, pois irq_lock()/irq_unlock() são funções nativas do Zephyr para controle de interrupções.
  Proteção de Seção Crítica (Main): Nas funções processamento_sensor_protegido() e processamento_sensor_otimizado(), adicionei irq_lock() antes de acessar a variável sensor_data e irq_unlock() somente após completar a escrita do novo valor. Isso encapsula toda a operação de leitura-processamento-escrita, impedindo que a ISR interrompa no meio do processamento.
  Proteção da Demonstração: Adicionei o par irq_lock()/irq_unlock() também na função demonstracao_operacao_segura() para garantir operações atômicas durante a demonstração explícita.
- Resultado após correção:
  A "Race Condition" (Condição de Corrida) foi eliminada. Não haverá mais "Data Corruption" (corrupção de dados).
  Antes: Main lia valor=100, ISR interrompia e atualizava para 150, Main continuava com valor antigo (100), calculava 125 e sobrescrevia a atualização da ISR.
  Agora: Main chama irq_lock(), lê valor=100 e processa. ISR tenta interromper mas fica bloqueada. Main calcula 100 + 25 = 125, escreve 125 e chama irq_unlock(). Só então a ISR pode executar e atualizar para 175.
  Log esperado: A sequência de valores do sensor será sempre consistente e as operações mostrarão "OPERAÇÃO SEGURA! Dados consistentes", sem corrupções detectadas.
- [Link para a print do resultado]: 
https://github.com/Poli-USP-Microcontroladores2/semaforo-race-condition-tecnicos-da-enel/blob/arthur/CONFIRMA%C3%87%C3%83O%20QUE%20DEU%20CERTO.png

--------------------------------------------------------

## Avaliação curta:
### Código do ARTHUR (avaliado por Gustavo): 
- O que estava errado antes:
- 		A instrução k_sleep estava no meio do processo da atualização da variável jantar, forçando troca de threads enquanto a variável ainda estava sendo manipulada. Isso fazia com que ambas as threads lessem o mesmo valor e incrementassem para o mesmo valor, repetindo números.
- O que mudou com a correção:
- 		Foi implementado um semáforo binário, então uma thread vai processar a variável até o fim quando pegar o semáforo, enquanto a outra thread espera, garantindo a resolução para o problema anterior. 
- Se o comportamento agora é estável:
- 		Sim, nenhuma thread consegue ler a variável jantar enquanto a outra estiver no processo de modificação, garantindo integridade dos dados.   

### Código do GUSTAVO (avaliado por Rafael):
- O que estava errado antes:
- 		A proteção fraca no valor do saldo a ser incrementado fazia com que ambas as threads tivessem a capacidade de incrementar o valor do saldo, porem com ocorrências de sobrescritas.
- O que mudou com a correção:
- 		Foi adicionado um mutex, fazendo assim o incremento na variavel global ser seguro
- Se o comportamento agora é estável:
- 		Agora os incrementos possuem uma parede de segurança proporcionada pelo mutex, assim, este momento delicado do codigo é assegurado, portanto, sim, o código agora é estavel.

### Código do RAFAEL (avaliado por Arthur):
- O que estava errado antes:
- 		Ocorria uma condição de corrida no padrão Read-Modify-Write. A ISR preemptava a Main Thread dentro da "janela de vulnerabilidade" (após a leitura dos dados, mas antes da escrita). Isso causava um Lost Update (atualização perdida): a Main Thread sobrescrevia os dados novos gerados pela ISR com resultados baseados em dados obsoletos.  
- O que mudou com a correção:
- 		Implementou-se uma Seção Crítica via mascaramento de interrupções (irq_lock). Isso transformou a manipulação dos dados compartilhados em uma operação atômica. Enquanto a Main Thread está na seção crítica, o escalonador e o hardware de interrupção são bloqueados, garantindo acesso exclusivo à memória.
- Se o comportamento agora é estável:
- 		A proteção assegura a Integridade Referencial: não é mais possível ler o estado do sistema pela metade ou corrompido. Então sim, o comportamento agora é estável.


