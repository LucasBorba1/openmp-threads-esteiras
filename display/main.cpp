#include <windows.h>
#include <iostream>
#include <omp.h>

void LerPipe() {
    HANDLE hPipe;
    char buffer[1024];
    DWORD dwLido;

    while (true) {
        while (true) {
            hPipe = CreateFile(
                TEXT("\\\\.\\pipe\\InfoDisplay"),
                GENERIC_READ,
                0,
                NULL,
                OPEN_EXISTING, 
                0,
                NULL);

            if (hPipe != INVALID_HANDLE_VALUE) {
                std::cout << "Conectado ao pipe." << std::endl;
                break;
            }

            DWORD dwError = GetLastError();
            if (dwError != ERROR_PIPE_BUSY) {
                std::cerr << "Não foi possível abrir o pipe. GLE=" << dwError << std::endl;
                Sleep(5000);
            } else {
                if (!WaitNamedPipe(TEXT("\\\\.\\pipe\\InfoDisplay"), 20000)) {
                    std::cerr << "Não foi possível abrir o pipe após espera: timeout" << std::endl;
                    Sleep(5000); 
                }
            }
        }


        std::cout << "Lendo do pipe..." << std::endl;
        while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwLido, NULL) != FALSE) {
            buffer[dwLido] = '\0';
            std::cout << "Recebido: " << buffer << std::endl;
        }

        DWORD dwReadError = GetLastError();
        if (dwReadError != ERROR_BROKEN_PIPE) {
            std::cerr << "Leitura do pipe falhou. GLE=" << dwReadError << std::endl;
        } else {
            std::cout << "Pipe desconectado." << std::endl;
        }

        CloseHandle(hPipe);
        Sleep(5000); 
    }
}

int main() {
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            LerPipe();
        }
    }
    return 0;
}
