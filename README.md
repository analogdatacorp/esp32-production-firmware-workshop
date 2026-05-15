# ESP32 Production Firmware Workshop
### by Analog Data | Rajath Kumar K S

Production-grade ESP32 firmware development 
using ESP-IDF, FreeRTOS, WiFi and MQTT.

## Quick Start

Clone this repository:
git clone https://github.com/analogdatacorp/esp32-production-firmware-workshop.git

## Prerequisites

- ESP-IDF v6.0.1 installed via EIM
- VS Code with ESP-IDF extension
- Any ESP32 board (DevKit / XIAO S3 / C3 SuperMini / PCB Cupid ESP32C6)
  or Wokwi simulator

## Repository Structure

day1/     — Day 1 sessions and Project 1
day2/     — Day 2 sessions and Projects 2 & 3
docs/     — Curriculum, setup guide, Wokwi links
resources/ — datasheets, cheatsheets

## Building a Project

cd day1/project1-sensor-pipeline
get_idf
idf.py set-target esp32
idf.py build
idf.py -p PORT flash monitor

## Wokwi Simulation

All projects include a wokwi/ folder.
Open diagram.json at wokwi.com to simulate 
without hardware.

## Workshop Details

Instructor: Rajath Kumar K S
Platform:   Zoom (Online)

analogdata.io | build.analogdata.io
