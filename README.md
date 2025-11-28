# PSI-Microcontroladores2-Aula06
Atividade: Resolução de Race Condition com Semáforo

## Revisão do código:
# Código do GUSTAVO (avaliado por Rafael):
# - [Link Código fonte com erros]: 
https://github.com/Poli-USP-Microcontroladores2/semaforo-race-condition-tecnicos-da-enel/blob/gustavo/src/main.c
# - Comportamento incorreto: 
- 	 Pelos LOGs Apresentados pelo sistema, o valor esperado, que era de 1.000.000 nunca era alcançado, sempre ficando aleatorizado entre valores maiores que 100.000 e menores que 1.000.000, isso ocorre por conta da race condition que ocorre nas linhas 54 e 75, onde as threads A e B tentam manipular o contador ao mesmo tempo e ocorre que sobrescrevem o valor aficionado pela outra thread, dessa forma, resultando em um saldo final menor que o o saldo esperado.
# - Momento do erro:
- 	O erro acontece durante a sequência de Leitura-Modificação-Escrita (Read-Modify-Write) não atômica dentro das threads A e B.

--------------------------------------------------------

## Casos de teste:
# Código do Rafael (feito por Rafael): 
- Pré-condição: 
	Serial monitor com Baud Rate de 11500, arquivo platformio.ini inserido no projeto, arquivo prj.conf inserido no projeto, conteúdo do arquivo códigoORIGINAL.c colado dentro do arquivo main.c do projeto 
- Etapas de teste: 
	Compilar e fazer upload do código para uma placa FRDM-KL25Z
- Pós-condição esperada: 
	 O timestamp regride de vez em quando, o que indica a ISR sobrescrita

--------------------------------------------------------

## Solução:
# Código do Rafael (feito por Rafael): 
- [Link Código fonte sem erros]: 
https://github.com/Poli-USP-Microcontroladores2/semaforo-race-condition-tecnicos-da-enel/blob/Rafael/codigocorrigido.c
-Mudanças feitas:
- 	Proteção com irq_lock()/irq_unlock(): Foi implementado o uso de irq_lock() e irq_unlock() para proteger as seções críticas onde os dados do sensor são acessados.
- 	Inicialização: Não foi necessária inicialização adicional, pois irq_lock()/irq_unlock() são funções nativas do Zephyr para controle de interrupções.
- 	proteção de Seção Crítica (Main): Nas funções processamento_sensor_protegido() e processamento_sensor_otimizado(), adicionei irq_lock() antes de acessar a variável sensor_data e irq_unlock() somente após completar a escrita do novo valor. Isso encapsula toda a operação de leitura-processamento-escrita, impedindo que a ISR interrompa no meio do processamento.
- 	proteção da Demonstração: Adicionei o par irq_lock()/irq_unlock() também na função demonstracao_operacao_segura() para garantir operações atômicas durante a demonstração explícita.
- Resultado após correção:
- 	A "Race Condition" (Condição de Corrida) foi eliminada. Não haverá mais "Data Corruption" (corrupção de dados).
- 	Antes: Main lia valor=100, ISR interrompia e atualizava para 150, Main continuava com valor antigo (100), calculava 125 e sobrescrevia a atualização da ISR.
- 	Agora: Main chama irq_lock(), lê valor=100 e processa. ISR tenta interromper mas fica bloqueada. Main calcula 100 + 25 = 125, escreve 125 e chama irq_unlock(). Só então a ISR pode executar e atualizar para 175.
- 	Log esperado: A sequência de valores do sensor será sempre consistente e as operações mostrarão "OPERAÇÃO SEGURA! Dados consistentes", sem corrupções detectadas.
- 	[Link para a print do resultado]: 
https://github.com/Poli-USP-Microcontroladores2/semaforo-race-condition-tecnicos-da-enel/blob/arthur/CONFIRMA%C3%87%C3%83O%20QUE%20DEU%20CERTO.png

--------------------------------------------------------

## Avaliação curta:
# Código do Gustavo (avaliado por Rafael):
- O que estava errado antes:  
- O que mudou com a correção:
- Se o comportamento agora é estável: 
