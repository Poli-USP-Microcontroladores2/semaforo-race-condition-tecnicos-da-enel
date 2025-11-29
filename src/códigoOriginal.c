#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(race_condition_demo, LOG_LEVEL_DBG);

// --- Configuração de LEDs (do seu código) ---
#define LED_A_NODE DT_ALIAS(led0) // LED verde
#define LED_B_NODE DT_ALIAS(led2) // LED vermelho
#define LED_C_NODE DT_ALIAS(led1) // LED azul (para o botão)

#define BUTTON_NODE DT_NODELABEL(user_button_0)

static const struct gpio_dt_spec ledA = GPIO_DT_SPEC_GET(LED_A_NODE, gpios);
static const struct gpio_dt_spec ledB = GPIO_DT_SPEC_GET(LED_B_NODE, gpios);
static const struct gpio_dt_spec ledc = GPIO_DT_SPEC_GET(LED_C_NODE, gpios);

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static struct gpio_callback button_cb_data;

// --- Configuração do Teste ---

// Para forçar a preempção e maximizar a race condition,
// vamos dar a ambas as threads a MESMA prioridade.
#define PRIO_THREAD 7 
#define INCREMENTS_PER_THREAD 500000

// --- O Recurso Compartilhado ---
// 'volatile' impede o compilador de otimizar demais o acesso
volatile int32_t shared_counter = 0;

// --- Semáforos para Sincronização do Teste ---
// Um para iniciar as threads
K_SEM_DEFINE(sem_start, 0, 2); // Começa em 0, máximo 2 (um para cada thread)
// Um para a main saber que as threads terminaram
K_SEM_DEFINE(sem_done, 0, 2);  // Começa em 0, máximo 2

// ----------------------------------------------------
// THREAD A — Tenta incrementar o contador
// ----------------------------------------------------
void thread_A(void *p1, void *p2, void *p3)
{
    while (1) {
        // 1. Espera o sinal (do botão) para começar
        k_sem_take(&sem_start, K_FOREVER);

        LOG_DBG("Thread A INICIANDO...");
        gpio_pin_set_dt(&ledA, 1); // Liga LED verde enquanto trabalha

        // 2. Seção Crítica (com Race Condition)
        for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
            shared_counter++; // <-- O PROBLEMA ESTÁ AQUI
        }

        gpio_pin_set_dt(&ledA, 0); // Desliga LED
        LOG_DBG("Thread A CONCLUIDA!");

        // 3. Avisa a main que terminou
        k_sem_give(&sem_done);
    }
}

// ----------------------------------------------------
// THREAD B — Também tenta incrementar o contador
// ----------------------------------------------------
void thread_B(void *p1, void *p2, void *p3)
{
    while (1) {
        // 1. Espera o sinal (do botão) para começar
        k_sem_take(&sem_start, K_FOREVER);

        LOG_DBG("Thread B INICIANDO...");
        gpio_pin_set_dt(&ledB, 1); // Liga LED vermelho enquanto trabalha

        // 2. Seção Crítica (com Race Condition)
        for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
            shared_counter++; // <-- O PROBLEMA ESTÁ AQUI
        }

        gpio_pin_set_dt(&ledB, 0); // Desliga LED
        LOG_DBG("Thread B CONCLUIDA!");

        // 3. Avisa a main que terminou
        k_sem_give(&sem_done);
    }
}

// ----------------------------------------------------
// Definição das threads
// ----------------------------------------------------
K_THREAD_DEFINE(a_tid, 1024, thread_A, NULL, NULL, NULL, PRIO_THREAD, 0, 0);
K_THREAD_DEFINE(b_tid, 1024, thread_B, NULL, NULL, NULL, PRIO_THREAD, 0, 0);

// ----------------------------------------------------
// ISR - Interrupção do Botão
// ----------------------------------------------------
void button_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    // Pisca o LED azul para dar feedback visual do clique
    gpio_pin_toggle_dt(&ledc);

    // Zera o contador para o novo teste
    shared_counter = 0;

    // Libera as duas threads para começarem a "correr"
    k_sem_give(&sem_start);
    k_sem_give(&sem_start);
}

// ----------------------------------------------------
// Função principal (agora é a monitora)
// ----------------------------------------------------
void main(void)
{
    // Inicializa GPIOs dos LEDs
    if (!device_is_ready(ledA.port) || !device_is_ready(ledB.port) || !device_is_ready(ledc.port)) {
        LOG_ERR("Dispositivos GPIO dos LEDs nao estao prontos.");
        return;
    }
    gpio_pin_configure_dt(&ledA, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ledB, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ledc, GPIO_OUTPUT_INACTIVE);

    // Configurar botão
    if (!device_is_ready(button.port)) {
        LOG_ERR("Dispositivo GPIO do botao nao esta pronto.");
        return;
    }
    gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);
    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_FALLING);
    gpio_init_callback(&button_cb_data, button_isr, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    LOG_INF("=== Demonstracao de RACE CONDITION ===");
    LOG_INF("Pressione o botao (SW0) para iniciar o teste.");
    LOG_INF("Valor esperado: %d\n", INCREMENTS_PER_THREAD * 2);

    while (1) {
        // 1. Espera a Thread A terminar
        k_sem_take(&sem_done, K_FOREVER);
        
        // 2. Espera a Thread B terminar
        k_sem_take(&sem_done, K_FOREVER);

        // 3. Ambas terminaram. Imprime o resultado.
        LOG_INF("--- Teste Concluido ---");
        LOG_INF("Valor Esperado: %d", INCREMENTS_PER_THREAD * 2);
        
        // Use LOG_ERR para destacar o resultado corrompido
        LOG_ERR("Valor Real:     %d", shared_counter);
        LOG_INF("Pressione o botao para testar novamente...\n");
    }
}