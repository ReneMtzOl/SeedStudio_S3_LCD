# SeedStudio WT32 3.5 Inch Display 

## Official Page
https://www.seeedstudio.com/WT32-3-5-Inch-Display-p-5542.html

## ESP32-S3
Standard ESP32-S3 with 16MB Flash attached in module

## Goals 
* I2C Periferal
    * Create an i2c_manager component
    * Get it to scan the bus as a function
    * Add every element on the bus as a funtion
    * Add explicitely an adress to the bus as a function (and a sanity check to see if that address exists)
*   Run the TP IC 
    * Get a response from the device
    * Create the component for the device
    * Read it trough i2c_manager