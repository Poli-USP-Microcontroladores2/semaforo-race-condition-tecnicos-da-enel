#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(race_fixed_robust, LOG_LEVEL_DBG);

// --- Hardware ---
#define LED_A_NODE DT_ALIAS(led0)
#define LED_B_NODE DT_ALIAS(led2)
#define LED_C_NODE DT_ALIAS(led1)
#define BUTTON_NODE DT_NODELABEL(user_button_0)

static const struct gpio_dt_spec ledA = GPIO_DT_SPEC_GET(LED_A_NODE, gpios);
static const struct gpio_dt_spec ledB = GPIO_DT_SPEC_GET(LED_B_NODE, gpios);
static const struct gpio_dt_spec ledc = GPIO_DT_SPEC_GET(LED_C_NODE, gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static struct gpio_callback button_cb_data;

// --- Configuração ---
#define PRIO_THREAD 7
#define INCREMENTS_PER_THREAD 100000 // Reduzido para testar mais rápido

volatile int32_t shared_counter = 0;
volatile bool test_is_running = false; // <--- NOVO: Flag de estado

K_SEM_DEFINE(sem_start_threads, 0, 2); // Libera as threads
K_SEM_DEFINE(sem_done, 0, 2);          // Threads avisam que acabaram
K_SEM_DEFINE(sem_trigger_test, 0, 1);  // Botão avisa a Main para começar

K_MUTEX_DEFINE(my_mutex);

// ----------------------------------------------------
// THREAD A
// ----------------------------------------------------
void thread_A(void *p1, void *p2, void *p3)
{
    while (1) {
        k_sem_take(&sem_start_threads, K_FOREVER); // Espera ordem da MAIN

        gpio_pin_set_dt(&ledA, 1);
        for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
            k_mutex_lock(&my_mutex, K_FOREVER);
            shared_counter++; 
            k_mutex_unlock(&my_mutex);
        }
        gpio_pin_set_dt(&ledA, 0);
        
        k_sem_give(&sem_done);
    }
}

// ----------------------------------------------------
// THREAD B
// ----------------------------------------------------
void thread_B(void *p1, void *p2, void *p3)
{
    while (1) {
        k_sem_take(&sem_start_threads, K_FOREVER); // Espera ordem da MAIN

        gpio_pin_set_dt(&ledB, 1);
        for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
            k_mutex_lock(&my_mutex, K_FOREVER);
            shared_counter++;
            k_mutex_unlock(&my_mutex);
        }
        gpio_pin_set_dt(&ledB, 0);

        k_sem_give(&sem_done);
    }
}

K_THREAD_DEFINE(a_tid, 1024, thread_A, NULL, NULL, NULL, PRIO_THREAD, 0, 0);
K_THREAD_DEFINE(b_tid, 1024, thread_B, NULL, NULL, NULL, PRIO_THREAD, 0, 0);

// ----------------------------------------------------
// ISR - Agora só avisa a Main
// ----------------------------------------------------
void button_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    // Se o teste já está rodando, ignora o botão (Debounce lógico simples)
    if (test_is_running) {
        return;
    }
    
    // Apenas avisa a main para começar. Não zera contadores, não libera threads.
    k_sem_give(&sem_trigger_test);
}

// ----------------------------------------------------
// Main - O "Maestro"
// ----------------------------------------------------
void main(void)
{
    // (Inicialização dos GPIOs omitida para brevidade - mantenha a sua igual)
    if (!device_is_ready(ledA.port) || !device_is_ready(ledB.port) || !device_is_ready(ledc.port) || !device_is_ready(button.port)) return;
    gpio_pin_configure_dt(&ledA, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ledB, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ledc, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);
    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_FALLING);
    gpio_init_callback(&button_cb_data, button_isr, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    LOG_INF("=== Sistema Robusto com MUTEX ===");
    LOG_INF("Pressione o botao para iniciar.");

    while (1) {
        // 1. Espera o botão ser apertado (ISR libera este semáforo)
        k_sem_take(&sem_trigger_test, K_FOREVER);

        // 2. Limpa qualquer 'sujeira' de bouncing do semáforo
        k_sem_reset(&sem_trigger_test);

        LOG_INF("--- Iniciando Teste ---");
        
        // 3. Configura o ambiente (Zona Segura - Single Threaded aqui)
        test_is_running = true;      // Bloqueia a ISR
        shared_counter = 0;          // Zera o contador com segurança
        gpio_pin_set_dt(&ledc, 1);   // LED Azul indica "Testando"

        // 4. Libera os cães (threads)
        k_sem_give(&sem_start_threads);
        k_sem_give(&sem_start_threads);

        // 5. Espera as threads terminarem
        k_sem_take(&sem_done, K_FOREVER);
        k_sem_take(&sem_done, K_FOREVER);

        // 6. Analisa resultado
        gpio_pin_set_dt(&ledc, 0);
        test_is_running = false; // Libera o botão para nova rodada

        int expected = INCREMENTS_PER_THREAD * 2;
        if (shared_counter == expected) {
            LOG_INF("SUCESSO: %d", shared_counter);
        } else {
            LOG_ERR("FALHA: %d (Esperado: %d)", shared_counter, expected);
        }
        
        // Pequeno delay para evitar repique no soltar do botão
        k_sleep(K_MSEC(500)); 
    }
}