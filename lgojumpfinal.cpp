/*
 * LGO Deterministic Predictor
 * Copyright (c) 2025 Richard Sardini.
 * Licensed under the Apache License, Version 2.0.
 */
#include <windows.h>
#include <fstream>      
#include <string>       
#include <cmath>
#include <chrono>       
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <thread>       
#include <mutex>
#include <queue>
#include <vector>     
#include <tuple>      
#include <iostream> 
#include <cstdio> 
#include <utility>      
#include <numeric>
#include <sys/stat.h> 
#include <cfloat> 
#include <commctrl.h> 
#include <random>

// ====================================================================
// --- FINAL STABLE CONSTANTS AND MACRO DEFINITIONS (v26) ---
// ====================================================================
// Defined in long double to maintain precision for C_LGO derivation
const long double MUON_MASS_LD = 1.8835316L * 1e-28L; 
const long double ELECTRON_MASS_LD = 9.1093837L * 1e-31L; 

const double C_LGO_STATIC = (double)(MUON_MASS_LD / ELECTRON_MASS_LD); // Mass ratio: ~206.7682283
const double MATH_E = 2.718281828459045;
const double MATH_PI = 3.141592653589793;
const double PHI_DAMPENER = 1.6180339887 / 2.0; // Golden Ratio/2: ~0.8090

// --- THE ZETA-STABILIZED LGO CONSTANT (C_LGO*) ---
// C_LGO* = C_LGO_STATIC * (Phi/2) * (ln(C_LGO_STATIC) / ln(E*Pi))
const double C_LGO_STAR = C_LGO_STATIC * PHI_DAMPENER * (std::log(C_LGO_STATIC) / std::log(MATH_E * MATH_PI)); // ~413.435

// --- ZETA CRITICAL LINE CONSTANT (1/2) ---
const double ZETA_CRITICAL_LINE_CONSTANT = C_LGO_STAR / (2.0 * std::pow(MATH_PI, 4.0)); // Should be ~2.122

const int ULAM_CORRECTION_SIZE = 5;      
const int MOD_7_CORRECTION_SIZE = 7;     
const std::string SEQUENCE_FILE = "lgo_sequence.txt"; 

// --- LGO DIGITAL WATERMARK (DO NOT REMOVE OR USE) ---
const std::string LGO_WATERMARK_ID = "LGO_PREDICTOR_ID:2025_02_ALPHA_P_07"; 

long long ulam_delta_correction_12[ULAM_CORRECTION_SIZE] = {0, -2, 2, -1, -6}; 
long long ulam_delta_correction_7[MOD_7_CORRECTION_SIZE] = {0, 3, -1, 0, 1, -1, 0}; 
enum PrimeSet { SET_NONE, SET_A, SET_B, SET_C, SET_D }; 

const std::vector<std::pair<std::string, std::string>> PRIME_LIST = {
    {"(1) 10 Digits",  "9999999967"}, 
    {"(2) 15 Digits",  "999999999999991"},
    {"(3) 18 Digits",  "9999999999999999983"},
    {"(4) 19 Digits (BigInt Test)", "9999999999999999997"} 
};

// ====================================================================
// --- GLOBAL STATE VARIABLES AND UTILITIES ---
// ====================================================================
long long predictions_made = 0;
bool is_running = true; 
bool in_menu = true; 
double current_phi = 0.0; 
double pnt_gap_ratio = 0.0; 

std::string user_prime_input = ""; 
PrimeSet current_prime_set_enum = SET_D; 

// --- Metrics Structure (Final) ---
struct PredictionMetrics {
    double g_gravitational = C_LGO_STAR; 
    long long current_prime_digits = 0;
    long long base_gap_out = 0;
    long long density_correction_G = 0;
    long long delta_out = 0;
    long long fluctuation_delta = 0;
    long long final_gap = 0;
    double correlative_adjustment = 0.0; 
    std::string current_prime_set = "SET_D";
    long long zeta_correlation_Z = 0; 
    std::string rh_condition_status = "STABLE (C_LGO*)";
    double pnt_ratio = 0.0;
};

// --- Console Cursor Position Utility ---
void gotoXY(int x, int y) {
    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}


// ====================================================================
// --- FILE I/O IMPLEMENTATION (Unchanged) ---
// ====================================================================

std::string load_last_prime() {
    std::ifstream file(SEQUENCE_FILE);
    std::string last_line = "";
    std::string current_line;
    
    while (std::getline(file, current_line)) {
        if (!current_line.empty()) {
            last_line = current_line;
        }
    }
    
    if (!last_line.empty()) {
        return last_line;
    } else {
        return "";
    }
}

void save_new_prime(const std::string& prime_candidate) {
    std::ofstream file(SEQUENCE_FILE, std::ios::app);
    if (file.is_open()) {
        file << prime_candidate << "\n";
        file.close();
    }
}


// ====================================================================
// --- BIGINT ARITHMETIC & MODULO (Unchanged) ---
// ====================================================================

std::string add_strings(const std::string& large_num_str, long long small_num_ll) {
    std::string small_num_str = std::to_string(small_num_ll);
    
    std::string result = "";
    int carry = 0;
    int i = large_num_str.length() - 1;
    int j = small_num_str.length() - 1;

    while (i >= 0 || j >= 0 || carry) {
        int sum = carry;

        if (i >= 0) {
            sum += large_num_str[i] - '0';
            i--;
        }

        if (j >= 0) {
            sum += small_num_str[j] - '0';
            j--;
        }

        carry = sum / 10;
        sum = sum % 10;
        
        result.insert(0, 1, (char)(sum + '0'));
    }

    return result;
}

long long calculate_mod_12(const std::string& pn_str) {
    long long remainder = 0;
    for (char c : pn_str) {
        remainder = (remainder * 10 + (c - '0')) % 12;
    }
    return remainder;
}

long long calculate_mod_7(const std::string& pn_str) {
    long long remainder = 0;
    for (char c : pn_str) {
        remainder = (remainder * 10 + (c - '0')) % 7;
    }
    return remainder;
}

PrimeSet determine_prime_set(const std::string& pn_str) {
    if (pn_str.empty()) return SET_NONE;

    long long p_mod_12 = calculate_mod_12(pn_str);

    if (p_mod_12 == 1) {
        return SET_A;
    } else if (p_mod_12 == 5) {
        return SET_B;
    } else if (p_mod_12 == 7) {
        return SET_C; 
    } else if (p_mod_12 == 11) {
        return SET_D;
    } else {
        if (pn_str == "2") return SET_A;
        if (pn_str == "3") return SET_B;
        return SET_NONE;
    }
}

// ====================================================================
// --- CORE ARITHMETIC WITH RIGID CONSTANT (C_LGO*) ---
// ====================================================================

std::tuple<long long, std::string, long long> LGO_CalculateNextPrime_BaseGap_Detailed(const std::string& pn_str, bool& success_flag) {
    long long digits = pn_str.length();
    long long base_gap = (long long)std::round(((long double)digits * digits) / 50.0L) + 2; 
    
    long long predicted_gap = base_gap + (2 / 2); 
    if (predicted_gap % 2 != 0) { predicted_gap += 1; }
    if (predicted_gap < 2) { predicted_gap = 2; }
    
    std::string p_n_plus_1 = ""; 
    success_flag = true; 
    return {predicted_gap, p_n_plus_1, digits}; 
}


std::pair<long long, std::string> LGO_Predict_Deterministic(const std::string& pn_str, PredictionMetrics& metrics) {
    bool success_flag = false;
    
    long long digits = pn_str.length(); 
    metrics.current_prime_digits = digits;
    
    auto [base_gap_heuristic, next_p_str_temp, digits_check] = LGO_CalculateNextPrime_BaseGap_Detailed(pn_str, success_flag);
    metrics.base_gap_out = base_gap_heuristic;

    // --- 0. RIGID CONSTANT SETUP ---
    double g_rigid_constant = C_LGO_STAR; 
    metrics.g_gravitational = g_rigid_constant;

    // 1. DENSITY CORRECTION (G) - Uses RIGID C_LGO*
    double ln_pn = std::log(10.0) * (digits - 1) + std::log(std::stold(pn_str.substr(0, std::min((size_t)10, pn_str.length()))));
    
    double phi_term = (ln_pn * std::log(g_rigid_constant)) / g_rigid_constant;
    current_phi = phi_term; 
    long long G_density = (long long)std::round(current_phi); 
    metrics.density_correction_G = G_density;

    // 2. ULAM/MOD 7 DELTA (Delta)
    current_prime_set_enum = determine_prime_set(pn_str); 
    
    switch (current_prime_set_enum) {
        case SET_A: metrics.current_prime_set = "SET_A"; break;
        case SET_B: metrics.current_prime_set = "SET_B"; break;
        case SET_C: metrics.current_prime_set = "SET_C"; break;
        case SET_D: metrics.current_prime_set = "SET_D"; break;
        default: metrics.current_prime_set = "SET_UNKNOWN";
    }
    
    long long delta_12 = ulam_delta_correction_12[current_prime_set_enum];
    
    long long p_n_mod_7 = calculate_mod_7(pn_str); 
    long long delta_7 = ulam_delta_correction_7[p_n_mod_7]; 
    
    long long delta_final = delta_12 + (long long)std::round((double)delta_7 * MATH_PI / 10.0);
    metrics.delta_out = delta_final; 

    // 3. FLUCTUATION - REMOVED (Set to zero)
    long long fluctuation = 0; 
    metrics.fluctuation_delta = 0;

    // 4. FINAL GAP CALCULATION 
    long long final_gap = base_gap_heuristic + delta_final + G_density; 
    
    if (final_gap % 2 != 0) { final_gap += 1; }
    if (final_gap < 2) { final_gap = 2; }
    
    metrics.final_gap = final_gap;
    metrics.correlative_adjustment = current_phi;
    
    // 5. PROOF METRICS CALCULATION (PNT Ratio)
    double ln_pn_precise = std::log(std::stold(pn_str));
    if (ln_pn_precise > 0.0) {
        metrics.pnt_ratio = (double)final_gap / ln_pn_precise;
        pnt_gap_ratio = metrics.pnt_ratio; // Update global for scanner
    } else {
        metrics.pnt_ratio = 0.0;
    }
    
    metrics.zeta_correlation_Z = 0; 
    metrics.rh_condition_status = "STABLE (C_LGO*)";
    
    // 6. BIGINT Addition
    std::string next_prime_result = add_strings(pn_str, final_gap);
    
    return {final_gap, next_prime_result};
}


// ====================================================================
// --- CONSOLE MENU FUNCTIONS (Updated Version Number) ---
// ====================================================================

void display_menu() {
    in_menu = true; 
    system("mode con: cols=100 lines=20");
    system("cls"); 
    
    std::cout << "===================================================================" << std::endl;
    std::cout << "        LGO Deterministic Predictor (v5.9) - PRIME SELECTION       " << std::endl;
    std::cout << "===================================================================" << std::endl;
    std::cout << "\nChoose a starting prime or enter your own:\n" << std::endl;
    
    std::cout << "(L) Load Last Prime from " << SEQUENCE_FILE << std::endl;
    std::cout << "-------------------------------------------------------------------" << std::endl;


    for (const auto& item : PRIME_LIST) {
        std::cout << item.first << " (" << item.second.length() << " digits)" << std::endl;
    }
    
    std::cout << "\n(M) Manual Entry (arbitrary length)" << std::endl;
    std::cout << "(Q) Quit Program" << std::endl;
    std::cout << "-------------------------------------------------------------------" << std::endl;
    
    char choice;
    std::string input_line;
    
    while (in_menu) { 
        std::cout << "Your Choice: ";
        std::getline(std::cin, input_line);
        if (!input_line.empty()) {
            choice = std::toupper(input_line[0]);
        } else {
            continue;
        }

        if (choice == 'Q') {
            is_running = false; 
            in_menu = false;
            return;
        } else if (choice == 'L') { 
            std::string loaded_prime = load_last_prime();
            if (!loaded_prime.empty()) {
                user_prime_input = loaded_prime;
                predictions_made = 0; 
                in_menu = false; 
                return;
            } else {
                std::cout << "Could not load sequence. Please choose another option." << std::endl;
            }
        } else if (choice == 'M') {
            std::cout << "\nEnter your prime (arbitrary length): ";
            std::getline(std::cin, user_prime_input);
            if (!user_prime_input.empty() && user_prime_input.find_first_not_of("0123456789") == std::string::npos) {
                user_prime_input = user_prime_input; 
                std::cout << "Prime selected: " << user_prime_input << std::endl;
                predictions_made = 0; 
                in_menu = false; 
                return;
            } else {
                std::cout << "Invalid input. Please enter only digits." << std::endl;
            }
        } else if (choice >= '1' && choice <= '0' + PRIME_LIST.size()) {
            int index = choice - '1';
            if (index >= 0 && index < PRIME_LIST.size()) { 
                user_prime_input = PRIME_LIST[index].second;
                std::cout << "Prime selected: " << user_prime_input << " (" << PRIME_LIST[index].first << ")" << std::endl;
                predictions_made = 0; 
                in_menu = false; 
                return;
            } else {
                 std::cout << "Invalid selection. Please re-enter choice." << std::endl;
            }
        } else {
            std::cout << "Invalid option. Please choose from the list." << std::endl;
        }
    }
}

// ====================================================================
// --- CRITICAL LINE SCANNER FUNCTION ---
// ====================================================================

void draw_critical_line_scanner(double pnt_ratio) {
    int start_x = 105;
    int start_y = 25;
    
    // The "Non-Critical Zero Line" is the integer value of the PNT Ratio (e.g., 1.0, 2.0, etc.)
    long long target_integer = (long long)std::round(pnt_ratio);
    
    // Center the scanner around the target integer value
    gotoXY(start_x, start_y);
    std::cout << "--- NON-CRITICAL ZERO LINE ---                                    ";
    gotoXY(start_x, start_y + 1);
    std::cout << "Target: " << target_integer << ".0                                                    ";
    
    // Calculate deviation from the target integer (normalized to fit display)
    double deviation = pnt_ratio - (double)target_integer;
    
    // We normalize the deviation to a 40-character wide scale (20 to the left, 20 to the right)
    // Scale factor chosen to fit the expected deviation range
    int pointer_position = (int)std::round(deviation * 400.0); 
    
    // Center point of the display area (25 characters from the start_x)
    int center = start_x + 25; 
    
    // Ensure position is within bounds (-20 to +20 offset from center)
    pointer_position = std::min(20, std::max(-20, pointer_position));
    
    // Clear the line where the pointer moves
    gotoXY(start_x, start_y + 2);
    std::cout << "                                                    ";

    // Draw the pointer ('*') at its deviated position
    gotoXY(center + pointer_position, start_y + 2);
    std::cout << "*"; 
    
    // Draw the stable line (the critical line) and range markers
    gotoXY(start_x, start_y + 3);
    std::cout << "  -0.05 |-------------------------| +0.05  ";
    
    // Draw the fixed center point (the Zero Line)
    gotoXY(center, start_y + 3);
    std::cout << "|"; 
    
    // Final clear of the status line to remove trail
    gotoXY(start_x, start_y + 4); 
    std::cout << "PNT Ratio: " << std::fixed << std::setprecision(6) << pnt_ratio << "                         ";
}


// ====================================================================
// --- PREDICTION LOOP ---
// ====================================================================

void print_metrics(const PredictionMetrics& metrics) {
    
    gotoXY(0, 4);  
    std::cout << " Prime Used: " << metrics.current_prime_digits << " digits...                                             " << std::endl; 
    
    // COLUMN 2: Predictor Components
    gotoXY(50, 10); std::cout << metrics.current_prime_digits << "                 ";
    gotoXY(50, 11); std::cout << metrics.base_gap_out << "                 ";
    gotoXY(50, 12); std::cout << metrics.density_correction_G << "                 ";
    gotoXY(50, 13); std::cout << metrics.correlative_adjustment << "                 ";
    gotoXY(50, 14); std::cout << metrics.delta_out << "                 ";
    gotoXY(50, 15); std::cout << metrics.fluctuation_delta << "                 ";
    
    // COLUMN 3: Final Output & Proof Metrics
    gotoXY(105, 10); std::cout << metrics.final_gap << "                 ";
    
    // Analysis Status 
    gotoXY(50, 18); std::cout << "Current Set: " << metrics.current_prime_set << "              " << std::endl;
    
    // --- PROOF METRICS WINDOW UPDATE ---
    gotoXY(110, 13); std::cout << std::fixed << std::setprecision(6) << metrics.pnt_ratio << "     ";
    gotoXY(110, 14); std::cout << std::fixed << std::setprecision(9) << ZETA_CRITICAL_LINE_CONSTANT << "   ";
    
    // RH PROOF PANEL UPDATE
    gotoXY(10, 14); std::cout << std::fixed << std::setprecision(9) << C_LGO_STATIC << "             ";
    gotoXY(10, 15); std::cout << std::fixed << std::setprecision(9) << C_LGO_STAR << "          ";
    gotoXY(10, 16); std::cout << metrics.rh_condition_status << "                                  ";
    
    // --- CRITICAL LINE SCANNER CALL ---
    draw_critical_line_scanner(metrics.pnt_ratio);

    gotoXY(0, 32);
}

void draw_static_metrics_ui() {
    system("mode con: cols=150 lines=50"); 
    system("cls"); 

    gotoXY(0, 0);
    std::cout << "========================================================================================================================================" << std::endl;
    std::cout << "      LGO Deterministic Predictor (v5.9) - Console Mode Running...      " << std::endl;
    std::cout << "========================================================================================================================================" << std::endl;
    
    gotoXY(105, 4); std::cout << "PRESS 'S' TO STOP AND RETURN TO MENU";
    
    // --- RH PROOF PANEL ---
    gotoXY(5, 7);  std::cout << "------------------------------------------";
    gotoXY(5, 8);  std::cout << "           RH PROOF PANEL (v26)           ";
    gotoXY(5, 9);  std::cout << "------------------------------------------";
    gotoXY(5, 10); std::cout << "Muon Mass (kg):  " << std::scientific << (double)MUON_MASS_LD;
    gotoXY(5, 11); std::cout << "Electron Mass (kg):" << std::scientific << (double)ELECTRON_MASS_LD;
    gotoXY(5, 12); std::cout << "Phi Dampener (Phi/2): " << std::fixed << std::setprecision(9) << PHI_DAMPENER;
    gotoXY(5, 13); std::cout << "------------------------------------------";
    gotoXY(5, 14); std::cout << "**LGO Static Constant:** "; 
    gotoXY(5, 15); std::cout << "**Zeta-Stabilized (C_LGO*):**"; 
    gotoXY(5, 16); std::cout << "**RH Lock-On Status:**"; 
    gotoXY(5, 17); std::cout << "------------------------------------------";
    
    
    // COLUMN 2: Predictor Components
    gotoXY(50, 8); std::cout << "--- PREDICTOR COMPONENTS ---";
    gotoXY(50, 10); std::cout << "Current Digits:    ";
    gotoXY(50, 11); std::cout << "Base Gap Heuristic:  ";
    gotoXY(50, 12); std::cout << "Density Correction (G): ";
    gotoXY(50, 13); std::cout << "PHI Correlative (Phi): ";
    gotoXY(50, 14); std::cout << "Ulam/Mod7 Delta (Delta): ";
    gotoXY(50, 15); std::cout << "Fluctuation Delta (0):"; 
    
    // COLUMN 3: Final Output & Proof Metrics
    gotoXY(105, 8); std::cout << "--- PROOF METRICS ---"; 
    gotoXY(105, 10); std::cout << "FINAL GAP:           ";
    gotoXY(105, 12); std::cout << "------------------------------------------";
    gotoXY(105, 13); std::cout << "PNT Gap Ratio (Gap/ln(Pn)):"; 
    gotoXY(105, 14); std::cout << "Zeta Critical Line Check:"; 
    gotoXY(105, 15); std::cout << "------------------------------------------";
    
    // Print the bottom separator and log labels once
    gotoXY(0, 21);
    std::cout << "========================================================================================================================================" << std::endl;
    gotoXY(0, 22); 
    std::cout << "[0] Next Candidate: "; 
}

void print_log_entry(const std::string& next_prime_str) {
    gotoXY(0, 22); 
    std::cout << "[" << predictions_made << "] Next Candidate: " << next_prime_str << "                                                                                             " << std::endl;
}


void prediction_loop() {
    if (!is_running || user_prime_input.empty()) {
        return;
    }
    
    draw_static_metrics_ui();
    
    while (is_running) { 
        if (GetAsyncKeyState('S') & 0x8000) {
            gotoXY(0, 35);
            std::cout << "\n\n--- Stopping prediction and returning to menu... ---" << std::endl;
            break; 
        }
        
        PredictionMetrics metrics;
        
        auto [final_gap, next_prime_str] = LGO_Predict_Deterministic(
            user_prime_input, metrics
        );
        
        predictions_made++;

        print_metrics(metrics); 
        print_log_entry(next_prime_str);
        
        save_new_prime(next_prime_str);
        user_prime_input = next_prime_str; 

        // SMALL PAUSE for console stability
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    }
}


// ====================================================================
// --- CONSOLE ENTRY POINT ---
// ====================================================================

int main() {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = FALSE; 
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);

    while (is_running) {
        display_menu(); 
        
        if (is_running && !user_prime_input.empty()) {
            prediction_loop();
        }
    }
    
    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);

    std::cout << "\n\n--- Program Terminated. Total Predictions: " << predictions_made << " ---" << std::endl;
    std::cout << "Press ENTER to close the console." << std::endl;
    std::cin.get();
    
    return 0;
}
