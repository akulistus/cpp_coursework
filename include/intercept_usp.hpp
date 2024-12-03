#ifndef INTERCETP_USB_H
#define INTERCETP_USB_H

#include <windows.h>
#include <vector>
#include <string>

// Function prototypes
void DetectAndConnectToArduino();
void ReadArduinoMessages();
void StartReadingArduino();
const std::vector<std::string>& GetArduinoMessages();

#endif // INTERCETP_USB_H
