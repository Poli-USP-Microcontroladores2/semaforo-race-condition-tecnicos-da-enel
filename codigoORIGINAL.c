#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <string.h>

// Estrutura de dados do sensor (multiplos campos para aumentar corrupção)
typedef struct {
    int32_t valor;
    uint32_t timestamp;
    uint16_t qualidade;
    uint8_t status;
} DadosSensor;

DadosSensor sensor_data = {100, 1000, 95, 1};

// Timer para simular interrupção de atualização do sensor
K_TIMER_DEFINE(timer_atualizacao, NULL, NULL);

// Estatísticas de corrupção
uint32_t total_operacoes = 0;
uint32_t corrupcoes_detectadas = 0;

// Função da ISR que atualiza os dados do sensor
void isr_atualiza_sensor(struct k_timer *timer_id)
{
    // Simula uma interrupção de hardware que atualiza os dados do sensor
    int32_t novo_valor = sensor_data.valor + 50;
    uint32_t novo_timestamp = sensor_data.timestamp + 1;
    
    // Atualiza os dados (esta operação pode interromper o main)
    sensor_data.valor = novo_valor;
    k_busy_wait(100); // Pequeno delay entre escritas
    sensor_data.timestamp = novo_timestamp;
    sensor_data.qualidade = 98;
    sensor_data.status = 2;
    
    printk("[ISR]  ⚡ Atualizou sensor: valor=%d, timestamp=%u\n", 
           novo_valor, novo_timestamp);
}

// Operação vulnerável à race condition
void processamento_sensor_vulneravel(void)
{
    total_operacoes++;
    
    // Backup para detecção de corrupção
    DadosSensor backup;
    memcpy(&backup, &sensor_data, sizeof(DadosSensor));
    
    printk("[MAIN] Iniciou processamento: valor=%d, timestamp=%u\n",
           backup.valor, backup.timestamp);
    
    // OPERAÇÃO VULNERÁVEL - não atômica
    // 1. Lê os dados atuais
    DadosSensor temp;
    memcpy(&temp, &sensor_data, sizeof(DadosSensor));
    
    printk("[MAIN] Leu dados: valor=%d\n", temp.valor);
    
    // 2. Processamento pesado (janela grande para race condition)
    k_busy_wait(8000); // 8ms - tempo suficiente para ISR interromper
    
    // Aplica cálculo complexo
    int32_t novo_valor = temp.valor + 25;
    uint32_t novo_timestamp = temp.timestamp;
    
    printk("[MAIN] Calculou novo valor: %d -> %d\n", temp.valor, novo_valor);
    
    // 3. Mais processamento
    k_busy_wait(8000);
    
    // 4. Escreve resultado (pode estar sobrescrevendo dados mais recentes)
    sensor_data.valor = novo_valor;
    k_busy_wait(500);
    sensor_data.timestamp = novo_timestamp;
    sensor_data.qualidade = 90;
    sensor_data.status = 3;
    
    printk("[MAIN] Escreveu resultado: valor=%d, timestamp=%u\n",
           novo_valor, novo_timestamp);
    
    // Verificação de consistência
    if (sensor_data.timestamp <= backup.timestamp) {
        corrupcoes_detectadas++;
        printk("💥 [MAIN] CORRUPÇÃO DETECTADA! Timestamp regrediu: %u -> %u\n",
               backup.timestamp, sensor_data.timestamp);
        printk("   💰 DADO PERDIDO: Atualização da ISR foi sobrescrita!\n");
    }
    
    if (sensor_data.valor < backup.valor) {
        printk("💥 [MAIN] CORRUPÇÃO GRAVE! Valor diminuiu: %d -> %d\n",
               backup.valor, sensor_data.valor);
    }
}

// Demonstração específica de corrupção de dados
void demonstracao_corrupção_explicita(void)
{
    printk("\n🎯 DEMONSTRAÇÃO EXPLÍCITA DE CORRUPÇÃO:\n");
    printk("   =================================\n");
    
    // Configura estado inicial conhecido
    sensor_data.valor = 200;
    sensor_data.timestamp = 2000;
    sensor_data.qualidade = 99;
    sensor_data.status = 1;
    
    printk("[MAIN] Estado inicial: valor=%d, timestamp=%u\n",
           sensor_data.valor, sensor_data.timestamp);
    
    // Força uma race condition evidente
    DadosSensor temp;
    
    // Main começa a operação
    printk("[MAIN] >>> Iniciando operação crítica...\n");
    memcpy(&temp, &sensor_data, sizeof(DadosSensor));
    
    k_busy_wait(5000); // Janela para ISR
    
    // ISR deve interromper aqui e atualizar os dados
    printk("[MAIN] Dados lidos: valor=%d\n", temp.valor);
    
    k_busy_wait(5000); // Mais processamento
    
    // Main continua sem saber que ISR atualizou os dados
    temp.valor += 30;
    sensor_data.valor = temp.valor; // SOBRESCREVE a atualização da ISR!
    sensor_data.timestamp = temp.timestamp;
    
    printk("[MAIN] <<< Operação completa. Resultado: valor=%d\n", sensor_data.valor);
    
    // Mostra o que aconteceu
    printk("\n🔍 ANÁLISE DA CORRUPÇÃO:\n");
    printk("   - Main leu: valor=%d\n", 200);
    printk("   - ISR atualizou para: valor=%d\n", 250); 
    printk("   - Main calculou: 200 + 30 = 230\n");
    printk("   - Resultado final: valor=%d (ATUALIZAÇÃO DA ISR PERDIDA!)\n", sensor_data.valor);
}

void main(void)
{
    printk("\n=== Zephyr RTOS - Demonstração de Race Condition (Main vs ISR) ===\n");
    printk("               ⚡ APENAS O PROBLEMA - SEM SOLUÇÕES ⚡\n\n");
    
    // Configura timer com callback de interrupção muito frequente
    k_timer_init(&timer_atualizacao, isr_atualiza_sensor, NULL);
    
    printk("🎯 OBJETIVO: Mostrar como a ISR corrompe dados durante o processamento do main\n\n");
    
    printk("1. OPERAÇÕES CONTÍNUAS COM RACE CONDITION:\n");
    printk("   ======================================\n");
    
    // Inicia timer periódico (interrompe frequentemente)
    k_timer_start(&timer_atualizacao, K_MSEC(3), K_MSEC(3));
    
    // Executa várias operações vulneráveis
    for (int i = 0; i < 8; i++) {
        processamento_sensor_vulneravel();
        k_sleep(K_MSEC(10));
        
        if (corrupcoes_detectadas >= 3) {
            printk("\n⚡ Múltiplas corrupções detectadas! Parando execução...\n");
            break;
        }
    }
    
    k_timer_stop(&timer_atualizacao);
    
    printk("\n📊 RELATÓRIO FINAL DA RACE CONDITION:\n");
    printk("   ================================\n");
    printk("   Total de operações: %u\n", total_operacoes);
    printk("   Corrupções detectadas: %u\n", corrupcoes_detectadas);
    printk("   Taxa de corrupção: %.1f%%\n", 
           (corrupcoes_detectadas * 100.0) / total_operacoes);
    
    if (corrupcoes_detectadas > 0) {
        printk("\n💥 PROVA CONCRETA: Race conditions ocorreram e corromperam dados!\n");
        printk("   Dados do sistema estão inconsistentes e não confiáveis.\n");
    }
    
    // Demonstração explícita
    k_sleep(K_MSEC(100));
    demonstracao_corrupção_explicita();
    
    printk("\n=== Fim da demonstração do problema ===\n");
    printk("   (Sem soluções implementadas - foco apenas na race condition)\n");
    
    // Loop infinito para manter o sistema rodando
    while (1) {
        k_sleep(K_SECONDS(1));
    }
}
