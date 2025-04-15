#include "ej1.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

/* -------------------- TESTS ORIGINALES DE LA CÁTEDRA -------------------- */

/**
 * Crea y destruye una lista vacía
 */
void test_create_destroy_list() {
    string_proc_list *list = string_proc_list_create_asm();
    string_proc_list_destroy(list);
}

/**
 * Crea y destruye un nodo
 */
void test_create_destroy_node() {
    string_proc_node *node = string_proc_node_create_asm(0, "hash");
    string_proc_node_destroy(node);
}

/**
 * Crea una lista y le agrega nodos
 */
void test_create_list_add_nodes() {
    string_proc_list *list = string_proc_list_create_asm();
    string_proc_list_add_node_asm(list, 0, "hola");
    string_proc_list_add_node_asm(list, 0, "a");
    string_proc_list_add_node_asm(list, 0, "todos!");
    string_proc_list_destroy(list);
}

/**
 * Crea una lista y le agrega nodos. Luego aplica la lista a un hash.
 */
void test_list_concat() {
    string_proc_list *list = string_proc_list_create();
    string_proc_list_add_node(list, 0, "hola");
    string_proc_list_add_node(list, 0, "a");
    string_proc_list_add_node(list, 0, "todos!");
    char *new_hash = string_proc_list_concat(list, 0, "hash");
    string_proc_list_destroy(list);
    free(new_hash);
}

/* -------------------- TESTS ADICIONALES -------------------- */

/**
 * Prueba la función de concatenación con una lista vacía
 */
void test_concat_empty_list() {
    string_proc_list *list = string_proc_list_create();
    char *new_hash = string_proc_list_concat(list, 0, "hash");
    
    if (new_hash != NULL && strcmp(new_hash, "hash") == 0) {
        printf("Test concat_empty_list: OK\n");
    } else {
        printf("Test concat_empty_list: FAIL\n");
    }
    
    string_proc_list_destroy(list);
    free(new_hash);
}

/**
 * Prueba la función con parámetros NULL
 */
void test_null_parameters() {
    string_proc_list *list = string_proc_list_create();
    
    // Prueba con lista NULL
    char *result1 = string_proc_list_concat(NULL, 0, "hash");
    if (result1 == NULL) {
        printf("Test null_parameters (NULL list): OK\n");
    } else {
        printf("Test null_parameters (NULL list): FAIL\n");
        free(result1);
    }
    
    // Prueba con hash NULL
    char *result2 = string_proc_list_concat(list, 0, NULL);
    if (result2 == NULL) {
        printf("Test null_parameters (NULL hash): OK\n");
    } else {
        printf("Test null_parameters (NULL hash): FAIL\n");
        free(result2);
    }
    
    string_proc_list_destroy(list);
}

/**
 * Prueba con diferentes tipos de nodos
 */
void test_different_node_types() {
    string_proc_list *list = string_proc_list_create();
    
    string_proc_list_add_node(list, 0, "tipo0_1");
    string_proc_list_add_node(list, 1, "tipo1_1");
    string_proc_list_add_node(list, 0, "tipo0_2");
    string_proc_list_add_node(list, 2, "tipo2_1");
    string_proc_list_add_node(list, 1, "tipo1_2");
    
    char *result_type0 = string_proc_list_concat(list, 0, "hash");
    char *result_type1 = string_proc_list_concat(list, 1, "hash");
    char *result_type2 = string_proc_list_concat(list, 2, "hash");
    
    if (result_type0 != NULL && strstr(result_type0, "tipo0_1") && strstr(result_type0, "tipo0_2") && 
        !strstr(result_type0, "tipo1_1") && !strstr(result_type0, "tipo2_1")) {
        printf("Test different_node_types (type 0): OK\n");
    } else {
        printf("Test different_node_types (type 0): FAIL\n");
    }
    
    if (result_type1 != NULL && strstr(result_type1, "tipo1_1") && strstr(result_type1, "tipo1_2") && 
        !strstr(result_type1, "tipo0_1") && !strstr(result_type1, "tipo2_1")) {
        printf("Test different_node_types (type 1): OK\n");
    } else {
        printf("Test different_node_types (type 1): FAIL\n");
    }
    
    if (result_type2 != NULL && strstr(result_type2, "tipo2_1") && 
        !strstr(result_type2, "tipo0_1") && !strstr(result_type2, "tipo1_1")) {
        printf("Test different_node_types (type 2): OK\n");
    } else {
        printf("Test different_node_types (type 2): FAIL\n");
    }
    
    string_proc_list_destroy(list);
    free(result_type0);
    free(result_type1);
    free(result_type2);
}

/**
 * Prueba con cadenas vacías
 */
void test_empty_strings() {
    string_proc_list *list = string_proc_list_create();
    
    string_proc_list_add_node(list, 0, "");
    string_proc_list_add_node(list, 0, "normal");
    string_proc_list_add_node(list, 0, "");
    
    char *result = string_proc_list_concat(list, 0, "hash");
    
    if (result != NULL && strcmp(result, "hashnormal") == 0) {
        printf("Test empty_strings: OK\n");
    } else {
        printf("Test empty_strings: FAIL\n");
    }
    
    string_proc_list_destroy(list);
    free(result);
}

/**
 * Prueba con una lista grande
 */
void test_large_list() {
    string_proc_list *list = string_proc_list_create();
    char buffer[20];
    char *str_copies[1000] = {NULL};
    int count = 0;
    
    for (int i = 0; i < 1000; i++) {
        sprintf(buffer, "node%d", i);
        str_copies[count] = malloc(strlen(buffer) + 1);
        if (str_copies[count]) {
            strcpy(str_copies[count], buffer);
            string_proc_list_add_node(list, i % 3, str_copies[count]);
            count++;
        }
    }
    
    char *result = string_proc_list_concat(list, 1, "hash");
    
    if (result != NULL) {
        printf("Test large_list: OK\n");
    } else {
        printf("Test large_list: FAIL\n");
    }
    
    free(result);
    
    for (int i = 0; i < count; i++) {
        free(str_copies[i]);
    }
    
    string_proc_list_destroy(list);
}

/**
 * Prueba con nodos que contienen hash NULL
 */
void test_null_hash_in_nodes() {
    string_proc_list *list = string_proc_list_create();
    
    string_proc_list_add_node(list, 0, "normal1");
    string_proc_list_add_node(list, 0, NULL);
    string_proc_list_add_node(list, 0, "normal2");
    
    char *result = string_proc_list_concat(list, 0, "hash");
    
    if (result != NULL && strstr(result, "normal1") && strstr(result, "normal2")) {
        printf("Test null_hash_in_nodes: OK\n");
    } else {
        printf("Test null_hash_in_nodes: FAIL\n");
    }
    
    string_proc_list_destroy(list);
    free(result);
}

/**
 * Prueba con caracteres especiales
 */
void test_special_characters() {
    string_proc_list *list = string_proc_list_create();
    
    string_proc_list_add_node(list, 0, "!@#$%^&*()");
    string_proc_list_add_node(list, 0, "áéíóú");
    string_proc_list_add_node(list, 0, "\\n\\t\\r");
    
    char *result = string_proc_list_concat(list, 0, "hash");
    
    if (result != NULL) {
        printf("Test special_characters: OK\n");
    } else {
        printf("Test special_characters: FAIL\n");
    }
    
    string_proc_list_destroy(list);
    free(result);
}

/**
 * Prueba de concatenación múltiple
 */
void test_multiple_concat() {
    string_proc_list *list = string_proc_list_create();
    
    string_proc_list_add_node(list, 0, "test");
    string_proc_list_add_node(list, 0, "multiple");
    string_proc_list_add_node(list, 0, "concat");
    
    char *result1 = string_proc_list_concat(list, 0, "hash");
    char *result2 = string_proc_list_concat(list, 0, "hash");
    
    if (result1 != NULL && result2 != NULL && strcmp(result1, result2) == 0) {
        printf("Test multiple_concat: OK\n");
    } else {
        printf("Test multiple_concat: FAIL\n");
    }
    
    string_proc_list_destroy(list);
    free(result1);
    free(result2);
}

/**
 * Prueba de tipos de nodos no existentes
 */
void test_nonexistent_type() {
    string_proc_list *list = string_proc_list_create();
    
    string_proc_list_add_node(list, 0, "tipo0");
    string_proc_list_add_node(list, 1, "tipo1");
    string_proc_list_add_node(list, 2, "tipo2");
    
    char *result = string_proc_list_concat(list, 99, "hash");
    
    if (result != NULL && strcmp(result, "hash") == 0) {
        printf("Test nonexistent_type: OK\n");
    } else {
        printf("Test nonexistent_type: FAIL\n");
    }
    
    string_proc_list_destroy(list);
    free(result);
}

/**
 * Prueba de rendimiento
 */
void test_performance() {
    string_proc_list *list = string_proc_list_create();
    char buffer[20];
    char *str_copies[5000] = {NULL};
    int count = 0;
    clock_t start, end;
    double cpu_time_used;
    
    for (int i = 0; i < 5000; i++) {
        sprintf(buffer, "node%d", i);
        str_copies[count] = malloc(strlen(buffer) + 1);
        if (str_copies[count]) {
            strcpy(str_copies[count], buffer);
            string_proc_list_add_node(list, i % 5, str_copies[count]);
            count++;
        }
    }
    
    start = clock();
    char *result = string_proc_list_concat(list, 2, "hash");
    end = clock();
    
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Test performance: Concatenation took %f seconds\n", cpu_time_used);
    
    free(result);
    
    for (int i = 0; i < count; i++) {
        free(str_copies[i]);
    }
    
    string_proc_list_destroy(list);
}

/**
 * Ejecuta todos los tests
 */
void run_tests() {
    /* Tests originales de la cátedra */
    test_create_destroy_list();
    test_create_destroy_node();
    test_create_list_add_nodes();
    test_list_concat();
    
    /* Tests adicionales para verificar robustez */
    printf("\n--- Ejecutando tests adicionales ---\n");
    test_concat_empty_list();
    test_null_parameters();
    test_different_node_types();
    test_empty_strings();
    test_large_list();
    test_null_hash_in_nodes();
    test_special_characters();
    test_multiple_concat();
    test_nonexistent_type();
    test_performance();
    printf("--- Tests adicionales completados ---\n");
}

int main(void) {
    run_tests();
    return 0;
}