#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdbool.h>
///////////////////////////////////////
typedef enum {
    STABLE,
    MODERATE_RISK,
    HIGH_RISK,
    CRITICAL,
    PREGNANT,
    ELDERLY
} MedicalCondition;
////////////////////////////////////////
typedef struct Patient {
    int patient_id;
    char name[50];
    int age;
    char gender[10];
    char contact_number[15];
    MedicalCondition condition;
    time_t arrival_time;
    int priority_score;
    char primary_symptoms[200];
    int vital_signs[3];
    struct Patient* next;
} Patient;
//////////////////////////////////////////
typedef struct {
    Patient* front;
    int total_patients;
    int waiting_patients;
} PriorityQueue;
//////////////////////////////////////////////////////////////
int available_doctors = 30;
int available_rooms = 60;
int available_equipment = 80;

///////////////////////////
Patient* create_patient(int id, const char* name, int age, const char* gender,
                        const char* contact, MedicalCondition condition,
                        const char* symptoms, int* vital_signs);
void calculate_priority(Patient* patient);
void enqueue_patient(PriorityQueue* queue, Patient* new_patient);
Patient* dequeue_patient(PriorityQueue* queue);
void release_patient(PriorityQueue* queue, const char* filename);
void display_patient_queue(PriorityQueue* queue);
void handle_command(PriorityQueue* queue, const char* cmd);
void add_patient(PriorityQueue* queue);
void estimate_wait_time(PriorityQueue* queue);
void check_medical_resources();
void check_room_availability();
void check_doctor_availability();
void clear_buffer();
void reserve_medical_resources();
void release_medical_resources();


Patient* create_patient(int id, const char* name, int age, const char* gender,
                        const char* contact, MedicalCondition condition,
                        const char* symptoms, int* vital_signs) {
    Patient* new_patient = malloc(sizeof(Patient));

    new_patient->patient_id = id;
    strncpy(new_patient->name, name, sizeof(new_patient->name) - 1);
    new_patient->age = age;
    strncpy(new_patient->gender, gender, sizeof(new_patient->gender) - 1);
    strncpy(new_patient->contact_number, contact, sizeof(new_patient->contact_number) - 1);
    new_patient->condition = condition;
    strncpy(new_patient->primary_symptoms, symptoms, sizeof(new_patient->primary_symptoms) - 1);

    for (int i = 0; i < 3; i++) {
        new_patient->vital_signs[i] = vital_signs[i];
    }

    new_patient->arrival_time = time(NULL);
    new_patient->next = NULL;

    calculate_priority(new_patient);

    return new_patient;
}

void calculate_priority(Patient* patient) {
    int priority = 0;

    if (patient->age < 5 || patient->age > 65) {
        priority += 20;
    }

    switch(patient->condition) {
        case CRITICAL:     priority += 50; break;
        case HIGH_RISK:    priority += 30; break;
        case MODERATE_RISK: priority += 15; break;
        case STABLE:       priority += 5;  break;
        case PREGNANT:     priority += 25; break;
        case ELDERLY:      priority += 20; break;
    }

    if (patient->vital_signs[0] > 100) priority += 10;
    if (patient->vital_signs[1] > 140) priority += 15;
    if (patient->vital_signs[2] < 90) priority += 15;

    time_t current_time = time(NULL);
    double wait_time = difftime(current_time, patient->arrival_time);
    priority += (int)(wait_time / 60);

    patient->priority_score = priority;
}
////////////////////////////////////
void enqueue_patient(PriorityQueue* queue, Patient* new_patient) {
    queue->total_patients++;
    queue->waiting_patients++;

    if (queue->front == NULL || new_patient->priority_score > queue->front->priority_score) {
        new_patient->next = queue->front;
        queue->front = new_patient;
        return;
    }

    Patient* current = queue->front;
    while (current->next != NULL &&
           current->next->priority_score >= new_patient->priority_score) {
        current = current->next;
    }

    new_patient->next = current->next;
    current->next = new_patient;
}
/////////////////////////////////////////////////
Patient* dequeue_patient(PriorityQueue* queue) {
    if (queue->front == NULL) return NULL;

    Patient* patient = queue->front;
    queue->front = queue->front->next;

    queue->total_patients--;
    queue->waiting_patients--;

    return patient;
}
//////////////////////////////////////////////////////////////////////////
void release_patient(PriorityQueue* queue, const char* filename) {

    Patient* patient = dequeue_patient(queue);
    if (patient == NULL) {
        printf("\033[38;2;152;251;152m Aucun patient à libérer. \033[0m\n");
        return;
    }

    FILE* file = fopen(filename, "a");
    if (file == NULL) {
        perror("Erreur d'ouverture du fichier");
        free(patient);
        return;
    }

    fprintf(file, "Patient ID: %d\n", patient->patient_id);
    fprintf(file, "Nom: %s\n", patient->name);
    fprintf(file, "Âge: %d\n", patient->age);
    fprintf(file, "Genre: %s\n", patient->gender);
    fprintf(file, "Contact: %s\n", patient->contact_number);
    fprintf(file, "Condition: %d\n", patient->condition);
    fprintf(file, "Symptômes: %s\n", patient->primary_symptoms);
    fprintf(file, "Signes vitaux: HR %d, BP %d, O2 %d\n",
            patient->vital_signs[0], patient->vital_signs[1], patient->vital_signs[2]);
    fprintf(file, "Priorité: %d\n", patient->priority_score);
    fprintf(file, "Heure d'arrivée: %s\n", ctime(&patient->arrival_time));
    fprintf(file, "----------------------------------------\n");

    fclose(file);

    free(patient);

    release_medical_resources();

    if (available_doctors > 0 && available_rooms > 0 && available_equipment > 0) {
        queue->waiting_patients--;
    }
}
//////////////////////////////////////////////////////
void display_patient_queue(PriorityQueue* queue) {
    if (queue->front == NULL) {
        printf("\033[38;2;152;251;152m Aucun patient dans la file d'attente. \033[0m\n");
        return;
    }

    Patient* current = queue->front;
    int position = 1;
    while (current != NULL) {
        printf("\033[38;2;152;251;152m Position %d: Patient ID: %d, Nom: %s, Priorite: %d \033[0m\n",
               position++, current->patient_id, current->name, current->priority_score);
        current = current->next;
    }
}
/////////////////////////////////////////////////
void handle_command(PriorityQueue* queue, const char* cmd) {
    if (strcmp(cmd, "AJOUTER_PATIENT") == 0) {
        add_patient(queue);
    } else if (strcmp(cmd, "ESTIMATION_DU_TEMPS") == 0) {
        estimate_wait_time(queue);
    } else if (strcmp(cmd, "LIBERER_PATIENT") == 0) {
        release_patient(queue, "patients_released.txt");
    } else if (strcmp(cmd, "AFFICHAGE_FILE_D'ATTENTE") == 0) {
        display_patient_queue(queue);
    } else if (strcmp(cmd, "DISPONIBILITE_MEDECIN") == 0) {
        check_doctor_availability();
    } else if (strcmp(cmd, "DISPONIBILITE_MATERIELS") == 0) {
        check_medical_resources();
    } else if (strcmp(cmd, "DISPONIBILITE_SALLE") == 0) {
        check_room_availability();
    } else {
        printf("\033[1;33m Commande inconnue.\033[0m\n");
    }
}
///////////////////////////////////////////
void add_patient(PriorityQueue* queue) {
    if (available_doctors <= 0 || available_rooms <= 0 || available_equipment <= 0) {
        printf("\033[1;31mErreur : Ressources medicales insuffisantes. Impossible d'ajouter le patient.\033[0m\n");
        return;
    }

    char name[50] = {0};
    char gender[10] = {0};
    char contact[15] = {0};
    char symptoms[200] = {0};
    int age = 0;
    int vital_signs[3] = {0};
    MedicalCondition condition = 0;

    while (1) {
        printf("Entrez le nom du patient : ");
        clear_buffer();
        if (fgets(name, sizeof(name), stdin) == NULL || name[0] == '\n') {
            printf("\033[1;33mErreur : Le nom ne peut pas être vide.\033[0m\n");
        } else {
            name[strcspn(name, "\n")] = '\0';

            int valid_name = 1;
            for (int i = 0; name[i]; i++) {
                if (!isalpha(name[i]) && name[i] != ' ') {
                    valid_name = 0;
                    break;
                }
            }

            if (valid_name) {
                break;
            }
            printf("\033[1;33mErreur : Le nom ne doit contenir que des lettres.\033[0m\n");
        }
    }

    while (1) {
        printf("Entrez l'age du patient : ");
        if (scanf("%d", &age) == 1 && age >= 0 && age <= 123) {
            break;
        }
        printf("\033[1;33mErreur : Age invalide. Plage autorisae : 0-123 ans.\033[0m\n");
        clear_buffer();
    }

    while (1) {
        printf("Entrez le genre du patient (Homme/Femme) : ");
        clear_buffer();
        fgets(gender, sizeof(gender), stdin);
        gender[strcspn(gender, "\n")] = '\0';

        if (strcasecmp(gender, "Homme") == 0 || strcasecmp(gender, "Femme") == 0 || strcasecmp(gender, "homme") == 0 || strcasecmp(gender, "femme") == 0) {
            break;
        }
        printf("\033[1;33mErreur : Genre invalide. Choisissez Homme ou Femme.\033[0m\n");
    }

    while (1) {
        printf("Entrez le numero de contact du patient : ");
        fgets(contact, sizeof(contact), stdin);
        contact[strcspn(contact, "\n")] = '\0';

        if (strlen(contact) == 8) {
            int valid_contact = 1;
            for (int i = 0; contact[i]; i++) {
                if (!isdigit(contact[i])) {
                    valid_contact = 0;
                    break;
                }
            }

            if (valid_contact) {
                break;
            }
        }
        printf("\033[1;33mErreur : Numero de contact invalide. Doit contenir 8 chiffres.\033[0m\n");
    }

    while (1) {
        printf("Choisissez la condition medicale :\n");
        printf("  0 : Stable\n");
        printf("  1 : Risque Modere\n");
        printf("  2 : Risque Eleve\n");
        printf("  3 : Critique\n");
        printf("  4 : Grossesse\n");
        printf("  5 : Senior\n");
        printf("Votre choix : ");
        if (scanf("%d", (int*)&condition) == 1 && condition >= 0 && condition <= 5) {
            break;
        }
        printf("\033[1;33mErreur : Condition medicale invalide.\033[0m\n");
        clear_buffer();
    }

    while (1) {
        printf("Entrez les symptomes : ");
        clear_buffer();
        fgets(symptoms, sizeof(symptoms), stdin);
        symptoms[strcspn(symptoms, "\n")] = '\0';

        if (strlen(symptoms) > 0) {
            break;
        }
        printf("\033[1;33mErreur : Les symptomes ne peuvent pas etre vides.\033[0m\n");
    }

    printf("Saisie des signes vitaux :\n");

    while (1) {
        printf("Entrez la frequence cardiaque (40-120 bpm) : ");
        if (scanf("%d", &vital_signs[0]) == 1 && vital_signs[0] >= 40 && vital_signs[0] <= 120) {
            break;
        }
        printf("\033[1;33mErreur : Frequence cardiaque invalide.\033[0m\n");
        clear_buffer();
    }

    while (1) {
        printf("Entrez la pression arterielle (40-200 mmHg) : ");
        if (scanf("%d", &vital_signs[1]) == 1 && vital_signs[1] >= 40 && vital_signs[1] <= 200) {
            break;
        }
        printf("\033[1;33mErreur : Pression arterielle invalide.\033[0m\n");
        clear_buffer();
    }

    while (1) {
        printf("Entrez la saturation en oxygene (80-100%) : ");
        if (scanf("%d", &vital_signs[2]) == 1 && vital_signs[2] >= 80 && vital_signs[2] <= 100) {
            break;
        }
        printf("\033[1;33mErreur : Saturation en oxygene invalide.\033[0m\n");
        clear_buffer();
    }

    reserve_medical_resources();

    int id = queue->total_patients + 1;
    Patient* new_patient = create_patient(id, name, age, gender, contact, condition, symptoms, vital_signs);
    enqueue_patient(queue, new_patient);

    printf("\033[38;2;60;179;113m Patient %s ajoute a la file d'attente avec succes.\033[0m\n", name);
}


////////////////////////////////////////////////////////////
void estimate_wait_time(PriorityQueue* queue) {
    if (queue->front == NULL) {
        printf("\033[38;2;152;251;152m Aucun patient en attente.\033[0m\n");
    } else {
        int wait_time = queue->front->priority_score / 2;
        printf("\033[38;2;152;251;152m Temps estime d'attente pour le prochain patient: %d minutes. \033[0m\n", wait_time);
    }
}

///////////////////////////////////////////
void check_doctor_availability() {
    printf("\033[38;2;152;251;152m Medecins disponibles: %d sur 30 \033[0m\n", available_doctors);
}

/////////////////////////////////////////
void check_medical_resources() {
    printf("\033[38;2;152;251;152m Materiels medicaux disponibles: %d sur 80 \033[0m\n", available_equipment);
}

/////////////////////////////////////////
void check_room_availability() {
    printf("\033[38;2;152;251;152m Salles disponibles: %d sur 60 \033[0m\n", available_rooms);
}
/////////////////////////////////////////
void clear_buffer() {
    while (getchar() != '\n');
}

////////////////////////////////////////
void reserve_medical_resources() {
    if (available_doctors > 0) {
        available_doctors--;
    }
    if (available_rooms > 0) {
        available_rooms--;
    }
    if (available_equipment > 0) {
        available_equipment--;
    }
}

/////////////////////////////////////////
void release_medical_resources() {
    available_doctors++;
    available_rooms++;
    available_equipment++;
}
/////////DECORATION//////////////
void print_header() {
    printf("\n");
    printf("\033[38;2;218;112;214m======== Gestion des urgences ========\033[0m\n");
    printf("\033[38;2;218;112;214m============================================\033[0m\n");
}

void print_footer() {
    printf("\033[38;2;218;112;214m==================================\033[0m\n");
    printf(" \033[38;2;218;112;214m Systeme d'urgence arrete. \033[0m\n");
}
/////////////////////////////////////////
int main() {
    PriorityQueue queue;
    queue.front = NULL;
    queue.total_patients = 0;
    queue.waiting_patients = 0;

    char command[50];
    print_header();
    while (true) {
        printf("\n\033[1;36m=========================\033[0m\n");
        printf("\033[1;36m  Entrez une commande :  \033[0m\n");
        printf("\033[1;36m=========================\033[0m\n");
        printf(" AJOUTER_PATIENT\n");
        printf(" ESTIMATION_DU_TEMPS\n");
        printf(" DISPONIBILITE_MEDECIN\n");
        printf(" DISPONIBILITE_MATERIELS\n");
        printf(" DISPONIBILITE_SALLE\n");
        printf(" LIBERER_PATIENT\n");
        printf(" AFFICHAGE_FILE_D'ATTENTE\n");
        printf(" QUITTER\n");
        printf("Votre choix : ");
        scanf("%s", command);

        if (strcmp(command, "QUITTER") == 0) {
            break;
        }

        handle_command(&queue, command);
    }
    print_footer();
    return 0;
}