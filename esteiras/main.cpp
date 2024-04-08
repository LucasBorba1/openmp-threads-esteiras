#include <windows.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <omp.h>
#include <conio.h>

std::vector<double> medidas;
double pesoTotal;
HANDLE hMutex;
volatile bool stopSignal = false;

void enviarPipe() {
    HANDLE hPipe = INVALID_HANDLE_VALUE;

    hPipe = CreateNamedPipe(
        TEXT("\\\\.\\pipe\\InfoDisplay"),
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1, 
        1024 * 16,
        1024 * 16,
        NMPWAIT_USE_DEFAULT_WAIT,
        NULL);

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Falha na criação do Pipe, GLE= " << GetLastError() << std::endl;
        return;
    }

    std::cout << "Pipe criado. Aguardando conexão do cliente..." << std::endl;


    while (true) {
        if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
            std::cout << "Cliente conectado, enviando dados..." << std::endl;
            while (!stopSignal) {
                std::ostringstream mensagem;
                DWORD dwWritten = 0;
                BOOL fSuccess = FALSE;

                WaitForSingleObject(hMutex, INFINITE);
                mensagem << "No. de Medidas: " << medidas.size() << " medidas\n"
                        << "Peso total (a cada 1500 medidas): " << pesoTotal << " kg";
                ReleaseMutex(hMutex);

                std::string info = mensagem.str();
                const char* buffer = info.c_str();

                fSuccess = WriteFile(hPipe, buffer, static_cast<DWORD>(strlen(buffer)), &dwWritten, NULL);

                if (!fSuccess || dwWritten == 0) {
                    std::cerr << "Escrita no pipe falhou, GLE= " << GetLastError() << std::endl;
                    break; 
                } else {
                    std::cout << "Informação enviada: " << info << std::endl;
                }

                Sleep(2000);
            }

            DisconnectNamedPipe(hPipe);
            std::cout << "Cliente desconectado. Aguardando nova conexão..." << std::endl;
        } else {
            std::cerr << "Conexão falhou, tentando novamente. GLE= " << GetLastError() << std::endl;
        }
    }
}


void esteira(double peso, double intervalo) {
    while (!stopSignal) {
        Sleep(static_cast<DWORD>(intervalo*1000));

        WaitForSingleObject(hMutex, INFINITE);
        medidas.push_back(peso);
        if (medidas.size() % 1500 == 0) {
            double soma1500 = 0;


            for (size_t i = medidas.size() - 1500; i < medidas.size(); ++i) {
                soma1500 += medidas[i];
            }

            pesoTotal += soma1500;
            std::cout << "Soma total: " << pesoTotal << std::endl;
        }
        ReleaseMutex(hMutex);
    }
}

void paradaEmergencia() {
    std::cout << "Pressione uma tecla para parar a execucao do programa\n";
    while (!stopSignal) {
        if (_kbhit()) {
            std::cout << "Parando o programa!\n";
            stopSignal = true;
            break;
        }
        Sleep(100);
    } 
}

int main() {

    hMutex = CreateMutex(NULL, FALSE, NULL);

    if (hMutex == NULL) {
        std::cerr << "Erro ao criar mutex: " << GetLastError() << std::endl;
        return 1;
    }

    #pragma omp parallel sections
    {
        #pragma omp section
        {
            esteira(5.0, 1.0);
        }
        #pragma omp section
        {
            esteira(2.0, 0.5);
        }
        #pragma omp section
        {
            esteira(0.5, 0.1);
        }
        #pragma omp section
        {
            enviarPipe();
        }
        #pragma omp section
        {
            paradaEmergencia();
        }
    }

    CloseHandle(hMutex);
    return 0;

}