# 🔔 Proyecto Timbre Inteligente

Repositorio oficial con la documentación, diseño y código del prototipo de timbre inteligente. Este proyecto se desarrolla para las materias de **Sistemas Embebidos 2** y **Prototipado Rápido**.

---

## 🎯 Objetivo del Proyecto

Diseñar y construir el prototipo de un timbre inteligente basado en una placa de circuito impreso (PCB) personalizada. El sistema captura la presencia de visitantes y gestiona la información de forma remota a través de una interfaz web centralizada.

---

## ⚙️ Arquitectura del Sistema

El proyecto destaca por su capacidad de comunicación remota. Los módulos **no necesitan estar conectados a la misma red Wi-Fi**, ya que la transferencia de datos se realiza a través de Internet.

### 1. Módulo del Timbre (Cliente)
* **Hardware:** ESP32-CAM integrado en una PCB propia diseñada en **KiCad**.
* **Firmware:** Desarrollado en el framework profesional **ESP-IDF** utilizando el entorno de **PlatformIO**.
* **Función:** Al presionar el timbre, el dispositivo se conecta a la red Wi-Fi local (Red A), captura una fotografía del visitante y la envía mediante una petición **HTTP POST** hacia el servidor remoto.

### 2. Módulo de Gestión (Servidor e Interfaz Web)
* **Hardware:** Raspberry Pi.
* **Backend:** Desarrollado en **Python**. Actúa como servidor web central conectado a Internet (Red B) para recibir las peticiones HTTP del timbre y procesar las imágenes.
* **Frontend:** Cuenta con una **interfaz web dedicada** que permite visualizar las fotografías tomadas y consultar los datos de los visitantes en tiempo real.
* **Almacenamiento:** Guarda el historial de visitas de forma estructurada en archivos independientes con formato `.csv` (un archivo `.csv` por cada timbre registrado).

---

## 📊 Datos Registrados

Por cada interacción con el timbre, el sistema procesa y despliega en la interfaz web:
* 📸 Fotografía del visitante.
* 📅 Fecha y hora exacta del evento.
* 🆔 Identificador único del timbre (soporte multi-timbre).

---

## 🛠️ Tecnologías y Herramientas

* **Hardware Base:** ESP32-CAM & Raspberry Pi
* **Diseño Electrónico:** KiCad (Diseño y ruteo de la placa PCB)
* **Entorno de Desarrollo (Firmware):** PlatformIO + ESP-IDF (C/C++)
* **Entorno de Desarrollo (Servidor):** Python
* **Protocolo de Comunicación:** HTTP (Transmisión optimizada de imágenes y metadatos)
* **Base de Datos:** Archivos de texto plano (.CSV) independientes por dispositivo