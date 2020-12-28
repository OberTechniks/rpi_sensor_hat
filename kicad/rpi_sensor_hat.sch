EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A 11000 8500
encoding utf-8
Sheet 1 1
Title "rpi_sensor_hat"
Date "2020-12-24"
Rev "0"
Comp "OberTechniks"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L rpi_sensor_hat:SSW-120-03-G-D J1
U 1 1 5FE5C81D
P 5200 3400
F 0 "J1" H 5250 4517 50  0000 C CNN
F 1 "SSW-120-03-G-D" H 5250 4426 50  0000 C CNN
F 2 "rpi_sensor_hat:ssw-120-03-g-d" H 5250 4425 50  0001 C CNN
F 3 "~" H 5200 3400 50  0001 C CNN
	1    5200 3400
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR?
U 1 1 5FE8334E
P 4750 4550
F 0 "#PWR?" H 4750 4300 50  0001 C CNN
F 1 "GND" H 4755 4377 50  0000 C CNN
F 2 "" H 4750 4550 50  0001 C CNN
F 3 "" H 4750 4550 50  0001 C CNN
	1    4750 4550
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR?
U 1 1 5FE85980
P 5750 4550
F 0 "#PWR?" H 5750 4300 50  0001 C CNN
F 1 "GND" H 5755 4377 50  0000 C CNN
F 2 "" H 5750 4550 50  0001 C CNN
F 3 "" H 5750 4550 50  0001 C CNN
	1    5750 4550
	1    0    0    -1  
$EndComp
Wire Wire Line
	5000 4400 4750 4400
Wire Wire Line
	4750 4400 4750 4550
Wire Wire Line
	5000 3700 4750 3700
Wire Wire Line
	4750 3700 4750 4400
Connection ~ 4750 4400
Wire Wire Line
	5000 2900 4750 2900
Wire Wire Line
	4750 2900 4750 3700
Connection ~ 4750 3700
Wire Wire Line
	5500 4100 5750 4100
Wire Wire Line
	5750 4100 5750 4550
Wire Wire Line
	5500 3900 5750 3900
Wire Wire Line
	5750 3900 5750 4100
Connection ~ 5750 4100
Wire Wire Line
	5500 3400 5750 3400
Wire Wire Line
	5750 3400 5750 3900
Connection ~ 5750 3900
Wire Wire Line
	5500 3100 5750 3100
Wire Wire Line
	5750 3100 5750 3400
Connection ~ 5750 3400
Wire Wire Line
	5500 2700 5750 2700
Wire Wire Line
	5750 2700 5750 3100
Connection ~ 5750 3100
$Comp
L power:+3V3 #PWR?
U 1 1 5FE89682
P 4600 2350
F 0 "#PWR?" H 4600 2200 50  0001 C CNN
F 1 "+3V3" H 4615 2523 50  0000 C CNN
F 2 "" H 4600 2350 50  0001 C CNN
F 3 "" H 4600 2350 50  0001 C CNN
	1    4600 2350
	1    0    0    -1  
$EndComp
$Comp
L power:+5V #PWR?
U 1 1 5FE8B282
P 5900 2350
F 0 "#PWR?" H 5900 2200 50  0001 C CNN
F 1 "+5V" H 5915 2523 50  0000 C CNN
F 2 "" H 5900 2350 50  0001 C CNN
F 3 "" H 5900 2350 50  0001 C CNN
	1    5900 2350
	1    0    0    -1  
$EndComp
Wire Wire Line
	4600 2500 4600 2350
Wire Wire Line
	4600 2500 5000 2500
Wire Wire Line
	5500 2500 5900 2500
Wire Wire Line
	5900 2500 5900 2350
Wire Wire Line
	5000 3300 4600 3300
Wire Wire Line
	4600 3300 4600 2500
Connection ~ 4600 2500
Wire Wire Line
	5500 2600 5900 2600
Wire Wire Line
	5900 2600 5900 2500
Connection ~ 5900 2500
Text Label 4000 2600 0    50   ~ 0
GPIO2_SDA
Text Label 4000 2700 0    50   ~ 0
GPIO3_SCL
Wire Wire Line
	4000 2600 5000 2600
Wire Wire Line
	4000 2700 5000 2700
Text Label 4000 2800 0    50   ~ 0
GPIO4_GPCLK0
Wire Wire Line
	5000 2800 4000 2800
Text Label 4000 3000 0    50   ~ 0
GPIO17
Wire Wire Line
	5000 3000 4000 3000
Text Label 4000 3100 0    50   ~ 0
GPIO27
Wire Wire Line
	5000 3100 4000 3100
Text Label 4000 3200 0    50   ~ 0
GPIO22
Wire Wire Line
	5000 3200 4000 3200
Text Label 4000 3400 0    50   ~ 0
GPIO10_MOSI
Wire Wire Line
	5000 3400 4000 3400
Text Label 4000 3500 0    50   ~ 0
GPIO9_MISO
Wire Wire Line
	5000 3500 4000 3500
Text Label 4000 3600 0    50   ~ 0
GPIO11_SCLK
Wire Wire Line
	5000 3600 4000 3600
Text Label 4000 3800 0    50   ~ 0
GPIO0_ID_SD
Wire Wire Line
	5000 3800 4000 3800
Text Label 4000 3900 0    50   ~ 0
GPIO5
Wire Wire Line
	5000 3900 4000 3900
Text Label 4000 4000 0    50   ~ 0
GPIO6
Wire Wire Line
	5000 4000 4000 4000
Text Label 4000 4100 0    50   ~ 0
GPIO13_PWM1
Wire Wire Line
	5000 4100 4000 4100
Text Label 4000 4200 0    50   ~ 0
GPIO19_PCM_FS
Wire Wire Line
	5000 4200 4000 4200
Text Label 4000 4300 0    50   ~ 0
GPIO26
Wire Wire Line
	5000 4300 4000 4300
Text Label 6000 2800 0    50   ~ 0
GPIO14_TXD
Wire Wire Line
	5500 2800 6000 2800
Text Label 6000 2900 0    50   ~ 0
GPIO15_RXD
Wire Wire Line
	5500 2900 6000 2900
Text Label 6000 3000 0    50   ~ 0
GPIO18_PCM_CLK
Wire Wire Line
	5500 3000 6000 3000
Text Label 6000 3200 0    50   ~ 0
GPIO23
Wire Wire Line
	5500 3200 6000 3200
Text Label 6000 3300 0    50   ~ 0
GPIO24
Wire Wire Line
	5500 3300 6000 3300
Text Label 6000 3500 0    50   ~ 0
GPIO25
Wire Wire Line
	5500 3500 6000 3500
Text Label 6000 3600 0    50   ~ 0
GPIO8_CE0
Wire Wire Line
	5500 3600 6000 3600
Text Label 6000 3700 0    50   ~ 0
GPIO7_CE1
Wire Wire Line
	5500 3700 6000 3700
Text Label 6000 3800 0    50   ~ 0
GPIO1_ID_SC
Wire Wire Line
	5500 3800 6000 3800
Text Label 6000 4000 0    50   ~ 0
GPIO12_PWM0
Wire Wire Line
	5500 4000 6000 4000
Text Label 6000 4200 0    50   ~ 0
GPIO16
Wire Wire Line
	5500 4200 6000 4200
Text Label 6000 4300 0    50   ~ 0
GPIO20_PCM_DIN
Wire Wire Line
	5500 4300 6000 4300
Text Label 6000 4400 0    50   ~ 0
GPIO21_PCM_DOUT
Wire Wire Line
	5500 4400 6000 4400
Wire Notes Line width 8
	3650 2000 3650 4950
Wire Notes Line width 8
	3650 4950 6900 4950
Wire Notes Line width 8
	6900 4950 6900 2000
Wire Notes Line width 8
	6900 2000 3650 2000
Text Notes 4450 2000 0    61   ~ 0
Connector for Raspberry Pi 4 Model B
$Comp
L MCU_ST_STM32L0:STM32L062K8Tx U?
U 1 1 5FE99240
P 8200 2900
F 0 "U?" H 8150 1811 50  0000 C CNN
F 1 "STM32L062K8Tx" H 8150 1720 50  0000 C CNN
F 2 "Package_QFP:LQFP-32_7x7mm_P0.8mm" H 7700 2000 50  0001 R CNN
F 3 "http://www.st.com/st-web-ui/static/active/en/resource/technical/document/datasheet/DM00108218.pdf" H 8200 2900 50  0001 C CNN
	1    8200 2900
	1    0    0    -1  
$EndComp
$Comp
L rpi_sensor_hat:FM25V10-G U?
U 1 1 5FEA3E28
P 8900 950
F 0 "U?" H 9175 965 50  0000 C CNN
F 1 "FM25V10-G" H 9175 874 50  0000 C CNN
F 2 "Package_SO:SOIC-8-1EP_3.9x4.9mm_P1.27mm_EP2.29x3mm" H 8900 950 50  0001 C CNN
F 3 "https://www.cypress.com/file/41691/download" H 8900 950 50  0001 C CNN
	1    8900 950 
	1    0    0    -1  
$EndComp
$Comp
L dk_Humidity-Moisture-Sensors:BME280 U?
U 1 1 5FEAAAE6
P 7750 4700
F 0 "U?" H 8078 4803 60  0000 L CNN
F 1 "BME280" H 8078 4697 60  0000 L CNN
F 2 "rpi_sensor_hat:Pressure_Sensor_LGA-8_2.5x2.5mm_BME280" H 7950 4900 60  0001 L CNN
F 3 "https://ae-bst.resource.bosch.com/media/_tech/media/datasheets/BST-BME280-DS002.pdf" H 7950 5000 60  0001 L CNN
F 4 "828-1063-1-ND" H 7950 5100 60  0001 L CNN "Digi-Key_PN"
F 5 "BME280" H 7950 5200 60  0001 L CNN "MPN"
F 6 "Sensors, Transducers" H 7950 5300 60  0001 L CNN "Category"
F 7 "Humidity, Moisture Sensors" H 7950 5400 60  0001 L CNN "Family"
F 8 "https://ae-bst.resource.bosch.com/media/_tech/media/datasheets/BST-BME280-DS002.pdf" H 7950 5500 60  0001 L CNN "DK_Datasheet_Link"
F 9 "/product-detail/en/bosch-sensortec/BME280/828-1063-1-ND/6136314" H 7950 5600 60  0001 L CNN "DK_Detail_Page"
F 10 "SENSOR PRESSURE HUMIDITY TEMP" H 7950 5700 60  0001 L CNN "Description"
F 11 "Bosch Sensortec" H 7950 5800 60  0001 L CNN "Manufacturer"
F 12 "Active" H 7950 5900 60  0001 L CNN "Status"
	1    7750 4700
	1    0    0    -1  
$EndComp
$EndSCHEMATC
