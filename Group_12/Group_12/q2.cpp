#include<iostream> // Include the IO stream library for input and output operations
#include<sys/ipc.h> // Include for IPC (Interprocess Communication) system calls
#include<sys/msg.h> // Include for message queue functions
#include<unistd.h> // Include for miscellaneous symbolic constants and types, and declarations for miscellaneous functions
#include<sys/wait.h> // Include for wait functions
#include<time.h> // Include for time manipulation functions
#include<bits/stdc++.h>
using namespace std;

struct message {
    long type; // Message type for message queue operations
    int data; // Actual data part of the message
};
typedef struct message message;

struct answer {
    long type; // Message type for message queue operations
    int ans[]; // Flexible array member for storing answers (Note: This is specific to C, and technically not valid in C++).
};
typedef struct answer answer;

// Function to randomly generate answers for questions
void get_answers(int answers[], int length){
    for (int i = 0; i < length; i++) {
        answers[i] = (rand()) % 4 + 1; // Assigns a random value between 1 and 4 to each answer
    }
}

int main(){

    srand(getpid()); // Seed the random number generator with the process ID to ensure different random sequences
    
    int no_students, no_questions; // Declare variables for the number of students and questions

    // Prompt and read number of students and questions from the user
    cout << "\n Enter the number of Students: ";
    cin >> no_students;
    cout << " Enter the number of Questions: ";
    cin >> no_questions;

    // Display a header for the correct answers section
    cout << "\n------------------------------------\n";
    cout << " Correct Answers Sheet\n";
    cout << "------------------------------------\n";
    
    int correct_answers[no_questions]; // Array to store the correct answers for each question
    get_answers(correct_answers, no_questions); // Populate the correct_answers array with random values
    for(int i=0; i<no_questions; i++){
        // Display the correct answer for each question
        cout << " Question " << i + 1 << " Correct Answer: " << correct_answers[i] << "\n";
    }
    cout << "------------------------------------\n\n";

    key_t key1, key2; // Variables for IPC keys
    int msg_id1, msg_id2; // Variables for message queue identifiers
    // Generate unique keys for message queues
    key1 = ftok("Message queue", 1);
    key2 = ftok("Message queue", 2);
    // Create message queues and get their IDs
    msg_id1 = msgget(key1, 0666 | IPC_CREAT);
    msg_id2 = msgget(key2, 0666 | IPC_CREAT);

    for(int i=1; i<=no_students; i++){
        int pid = fork(); // Fork a new process for each student
        if(pid == -1){
            // Error handling for fork failure
            cerr << "Error: Unable to create process for student " << i << ".\n";
            exit(-1);
        }
        if(pid == 0){
            // Child process block
            srand(getpid()); // Re-seed the random number generator for the child process
            message m; // Instantiate a message structure for communication
            cout << "\n[Student " << i << " Process Initiated]\n";
            if(msgrcv(msg_id1, &m, sizeof(int), i, 0) == -1){
                // Error handling for message receive failure
                cerr << "Error: Unable to receive questions for Student " << i << ".\n";
                exit(-1);
            }
            cout << " Student " << i << " received " << m.data << " questions.\n";

            answer *ans = (answer *)malloc(sizeof(answer) + m.data * sizeof(int)); // Dynamically allocate memory for the answers
            ans->type = i; // Set the message type
            get_answers(ans->ans, m.data); // Populate the answer array with random answers
            cout << " Student " << i << " Answers:\n";
            for(int j=0; j<no_questions; j++){
                // Display each answer being sent
                cout << "  Question " << j + 1 << ": Answer = " << ans->ans[j] << "\n";
            }
            cout << "\n[Answers Sent by Student " << i << "]\n";
            if(msgsnd(msg_id2, ans, m.data * sizeof(int), 0) == -1) {
                // Error handling for message send failure
                cerr << "Error: Unable to send answers for Student " << i << ".\n";
                exit(1);
            }
            exit(0); // Exit the child process successfully
        }
    }

    // Display section header for sending questions to students
    cout << "\n====================================\n";
    cout << " Sending Questions to Students\n";
    cout << "====================================\n";

    for(int i=1; i<=no_students; i++) {
        // Prepare and send a message with the number of questions to each student
        message m;
        m.type = i; // Set the message type
        m.data = no_questions; // Assign the number of questions
        if(msgsnd(msg_id1, &m, sizeof(int), 0) == -1){
            // Error handling for message send failure
            cerr << "Error: Unable to send question message to Student " << i << ".\n";
            exit(-1);
        }
        cout << " Questions sent to Student " << i << ".\n"; // Indicate questions sent
    }

    for(int i=1; i<=no_students; i++) {
        wait(NULL); // Wait for all child processes to terminate
    }

    // Display section header for grading students' answers
    cout << "\n====================================\n";
    cout << " Grading Students' Answers\n";
    cout << "====================================\n";

    float grades[no_students+1] = {0}; // Initialize an array to store grades for each student
    float avg_grade=0; // Variable to calculate the average grade

    for(int i=1; i<=no_students; i++) {
        // Allocate memory for receiving answers and grade them
        answer *ans = (answer *)malloc(sizeof(answer) + no_questions * sizeof(int));
        if(msgrcv(msg_id2, ans, no_questions * sizeof(int), i, 0) == -1){
            // Error handling for message receive failure
            cerr << "Error: Unable to receive answers for grading, Student " << i << ".\n";
            exit(-1);
        }

        cout << "\n[Grading Student " << i << "]\n";
        for(int j=0; j<no_questions; j++){
            // Display each received answer and compare it with the correct answer
            cout << " Question " << j + 1 << ": Answer = " << ans->ans[j] << ", Correct Answer = " << correct_answers[j] << "\n";
            grades[i]+=(correct_answers[j]==ans->ans[j]); // Calculate score based on correct answers
        }
        grades[i] = (grades[i]/no_questions)*100; // Calculate and display the percentage grade for each student
        cout << " Final Grade: " << grades[i] << "%\n";
        avg_grade+=grades[i]; // Add to the total grades for average calculation
    }

    avg_grade = avg_grade/no_students; // Calculate the overall average grade
    // Display the overall average grade
    cout << "\n====================================\n";
    cout << " Overall Average Grade: " << avg_grade << "%\n";
    cout << "====================================\n";

    // Remove the message queues after use
    msgctl(msg_id1, IPC_RMID, NULL);
    msgctl(msg_id2, IPC_RMID, NULL);

    return 0; // Program completed successfully
}

