#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

// O CÓDIGO EM PY ORIGINAL FOI TRADUZIDO PELA IA PARA C-ZEPHYR E, CONSEQUENTEMENTE, FOI LEVEMENTE MODIFICADO PELA IA

// Registro do módulo de log
LOG_MODULE_REGISTER(CHECAR_JANTAR, LOG_LEVEL_INF);

// Variável compartilhada - representa o estado do jantar
volatile int jantar = 0;

// Thread IDs
static k_tid_t chloe_tid, david_tid, verificador_tid;

// Declarações das threads
static struct k_thread chloe_thread, david_thread, verificador_thread;

// Stack das threads
#define STACK_SIZE 1024
K_THREAD_STACK_DEFINE(chloe_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(david_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(verificador_stack, STACK_SIZE);

// Função do chefe Chloe
void chefe_chloe(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);
    
    while (1) {
        // Race condition: lê o valor atual
        int valor_atual = jantar;
        
        // Simula algum processamento
        k_sleep(K_MSEC(100));
        
        // Race condition: atualiza baseado no valor antigo
        jantar = valor_atual + 1;
        
        LOG_INF("Chloe adicionou sua etapa. Jantar agora: %d", jantar);
        
        k_sleep(K_MSEC(500));
    }
}

// Função do chefe David
void chefe_david(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);
    
    while (1) {
        // Race condition: lê o valor atual
        int valor_atual = jantar;
        
        // Simula algum processamento
        k_sleep(K_MSEC(150));
        
        // Race condition: atualiza baseado no valor antigo
        jantar = valor_atual + 1;
        
        LOG_INF("David adicionou sua etapa. Jantar agora: %d", jantar);
        
        k_sleep(K_MSEC(500));
    }
}

// Função que verifica o estado do jantar
void verificador_jantar(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);
    
    while (1) {
        k_sleep(K_SECONDS(1.7));
        
        // Verifica se o jantar está no ponto (par) ou cru (ímpar)
        if (jantar % 2 == 0) {
            LOG_INF("=== JANTAR NO PONTO! Valor: %d (par) ===", jantar);
        } else {
            LOG_ERR("=== JANTAR CRU! Valor: %d (ímpar) ===", jantar);
        }
    }
}

void main(void)
{
    LOG_INF("Iniciando preparo do jantar...");
    LOG_INF("Jantar inicial: %d", jantar);
    
    // Cria as threads dos chefs
    chloe_tid = k_thread_create(
        &chloe_thread,
        chloe_stack,
        K_THREAD_STACK_SIZEOF(chloe_stack),
        chefe_chloe,
        NULL, NULL, NULL,
        5, 0, K_NO_WAIT
    );
    
    david_tid = k_thread_create(
        &david_thread,
        david_stack,
        K_THREAD_STACK_SIZEOF(david_stack),
        chefe_david,
        NULL, NULL, NULL,
        5, 0, K_NO_WAIT
    );
    
    // Cria a thread do verificador
    verificador_tid = k_thread_create(
        &verificador_thread,
        verificador_stack,
        K_THREAD_STACK_SIZEOF(verificador_stack),
        verificador_jantar,
        NULL, NULL, NULL,
        4, 0, K_NO_WAIT
    );
    
    LOG_INF("Todas as threads foram criadas!");
    
    // Fica rodando eternamente
    while (1) {
        k_sleep(K_SECONDS(10));
    }
}
