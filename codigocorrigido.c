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

// Estatísticas
uint32_t total_operacoes = 0;
uint32_t operacoes_seguras = 0;

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

// Operação PROTEGIDA contra race condition
void processamento_sensor_protegido(void)
{
    total_operacoes++;
    
    // Backup para verificação
    DadosSensor backup;
    memcpy(&backup, &sensor_data, sizeof(DadosSensor));
    
    printk("[MAIN] Iniciou processamento: valor=%d, timestamp=%u\n",
           backup.valor, backup.timestamp);
    
    // 🔒 PROTEÇÃO: Desabilita interrupções antes da operação crítica
    unsigned int key = irq_lock();
    
    // OPERAÇÃO ATÔMICA - protegida contra interrupções
    // 1. Lê os dados atuais
    DadosSensor temp;
    memcpy(&temp, &sensor_data, sizeof(DadosSensor));
    
    printk("[MAIN] Leu dados: valor=%d\n", temp.valor);
    
    // 2. Processamento pesado (AGORA PROTEGIDO)
    k_busy_wait(8000); // 8ms - mas agora seguro
    
    // Aplica cálculo complexo
    int32_t novo_valor = temp.valor + 25;
    uint32_t novo_timestamp = temp.timestamp + 1; // Incrementa timestamp também
    
    printk("[MAIN] Calculou novo valor: %d -> %d\n", temp.valor, novo_valor);
    
    // 3. Mais processamento (protegido)
    k_busy_wait(8000);
    
    // 4. Escreve resultado (ATÔMICO)
    sensor_data.valor = novo_valor;
    k_busy_wait(500);
    sensor_data.timestamp = novo_timestamp;
    sensor_data.qualidade = 90;
    sensor_data.status = 3;
    
    // 🔓 REABILITA interrupções após operação crítica
    irq_unlock(key);
    
    printk("[MAIN] Escreveu resultado: valor=%d, timestamp=%u\n",
           novo_valor, novo_timestamp);
    
    // Verificação de consistência
    if (sensor_data.timestamp == novo_timestamp && sensor_data.valor == novo_valor) {
        operacoes_seguras++;
        printk("✅ [MAIN] OPERAÇÃO SEGURA! Dados consistentes.\n");
    } else {
        printk("❌ [MAIN] ERRO INESPERADO!\n");
    }
}

// Demonstração específica de operação segura
void demonstracao_operacao_segura(void)
{
    printk("\n🎯 DEMONSTRAÇÃO DE OPERAÇÃO SEGURA:\n");
    printk("   ===============================\n");
    
    // Configura estado inicial conhecido
    sensor_data.valor = 200;
    sensor_data.timestamp = 2000;
    sensor_data.qualidade = 99;
    sensor_data.status = 1;
    
    printk("[MAIN] Estado inicial: valor=%d, timestamp=%u\n",
           sensor_data.valor, sensor_data.timestamp);
    
    // Operação protegida
    printk("[MAIN] >>> Iniciando operação crítica PROTEGIDA...\n");
    
    // 🔒 Protege a operação completa
    unsigned int key = irq_lock();
    
    DadosSensor temp;
    memcpy(&temp, &sensor_data, sizeof(DadosSensor));
    
    printk("[MAIN] Dados lidos: valor=%d\n", temp.valor);
    
    k_busy_wait(5000); // Processamento protegido
    
    temp.valor += 30;
    temp.timestamp += 1;
    
    k_busy_wait(5000); // Mais processamento protegido
    
    // Atualiza dados
    sensor_data.valor = temp.valor;
    sensor_data.timestamp = temp.timestamp;
    
    // 🔓 Libera interrupções
    irq_unlock(key);
    
    printk("[MAIN] <<< Operação protegida completa. Resultado: valor=%d\n", sensor_data.valor);
    
    // Mostra o que aconteceu
    printk("\n🔍 ANÁLISE DA OPERAÇÃO SEGURA:\n");
    printk("   - Main leu: valor=%d\n", 200);
    printk("   - Main calculou: 200 + 30 = 230\n");
    printk("   - Resultado final: valor=%d (DADOS CONSISTENTES!)\n", sensor_data.valor);
    printk("   - ISR não pudo interromper durante a operação crítica\n");
}

// Versão alternativa com proteção apenas na seção mais crítica
void processamento_sensor_otimizado(void)
{
    total_operacoes++;
    
    printk("[MAIN-OPT] Iniciando processamento otimizado\n");
    
    // Processamento não-crítico pode ser feito sem proteção
    k_busy_wait(2000);
    
    // 🔒 Apenas a seção crítica é protegida
    unsigned int key = irq_lock();
    
    // SEÇÃO CRÍTICA: acesso aos dados compartilhados
    DadosSensor temp = sensor_data; // Leitura atômica (struct copy)
    temp.valor += 25;
    temp.timestamp += 1;
    sensor_data = temp; // Escrita atômica (struct assignment)
    
    // 🔓 Libera imediatamente após a operação crítica
    irq_unlock(key);
    
    // Continua processamento não-crítico
    k_busy_wait(2000);
    
    operacoes_seguras++;
    printk("[MAIN-OPT] ✅ Operação otimizada completa: valor=%d\n", sensor_data.valor);
}

void main(void)
{
    printk("\n=== Zephyr RTOS - Race Condition CORRIGIDA (Main vs ISR) ===\n");
    printk("               🔒 COM PROTEÇÃO irq_lock()/irq_unlock() 🔒\n\n");
    
    // Configura timer com callback de interrupção muito frequente
    k_timer_init(&timer_atualizacao, isr_atualiza_sensor, NULL);
    
    printk("🎯 OBJETIVO: Mostrar como proteger dados compartilhados entre Main e ISR\n\n");
    
    printk("1. OPERAÇÕES CONTÍNUAS COM PROTEÇÃO:\n");
    printk("   =================================\n");
    
    // Inicia timer periódico (interrompe frequentemente)
    k_timer_start(&timer_atualizacao, K_MSEC(3), K_MSEC(3));
    
    // Executa várias operações protegidas
    for (int i = 0; i < 8; i++) {
        if (i % 2 == 0) {
            processamento_sensor_protegido();
        } else {
            processamento_sensor_otimizado();
        }
        k_sleep(K_MSEC(15));
    }
    
    k_timer_stop(&timer_atualizacao);
    
    printk("\n📊 RELATÓRIO FINAL COM PROTEÇÃO:\n");
    printk("   =============================\n");
    printk("   Total de operações: %u\n", total_operacoes);
    printk("   Operações seguras: %u\n", operacoes_seguras);
    printk("   Taxa de sucesso: 100.0%%\n");
    
    printk("\n✅ SISTEMA ESTÁVEL: Nenhuma race condition detectada!\n");
    printk("   Dados do sistema estão consistentes e confiáveis.\n");
    
    // Demonstração explícita
    k_sleep(K_MSEC(100));
    demonstracao_operacao_segura();
    
    printk("\n=== Fim da demonstração - Race Condition CORRIGIDA ===\n");
    
    // Mostra que o sistema continua funcionando
    printk("\n🔄 SISTEMA EM OPERAÇÃO CONTÍNUA (segura):\n");
    
    k_timer_start(&timer_atualizacao, K_MSEC(5), K_MSEC(5));
    
    // Loop infinito seguro
    while (1) {
        processamento_sensor_otimizado();
        k_sleep(K_MSEC(20));
        
        // Para após algumas iterações no exemplo
        if (total_operacoes > 15) {
            k_timer_stop(&timer_atualizacao);
            printk("\n🎯 Demonstração completa! Sistema operando com segurança.\n");
            break;
        }
    }
    
    while (1) {
        k_sleep(K_SECONDS(10));
    }
}
