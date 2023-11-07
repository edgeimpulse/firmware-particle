# Edge Impulse firmware for Particle Photon 2

The Particle Photon 2 is a development module with a microcontroller and Wi-Fi networking. The board itself doesn't include any sensors, but we added examples for both microphone and accelerometer.

## Setup the sensors

The firmware is based on following sensors:
- [Adafruit](https://www.adafruit.com/product/3492) PDM microphone
- [Analog Devices](https://www.analog.com/en/products/adxl362.html) ADXL362 MEMS accelerometer

The libraries used to control and get data from the sensors are already included in this repository.

How to connect the hardware can be found on our [docs page](https://docs.edgeimpulse.com/docs/development-platforms/officially-supported-mcu-targets/particle-photon-2) for the Photon 2.


## Replacing the ML model

By default the firmware includes the [motion recognition](https://docs.edgeimpulse.com/docs/tutorials/end-to-end-tutorials/continuous-motion-recognition) model. You can replace this model with a your model deployed in Edge Impulse. Following steps show how you can replace the model:

1. In Edge Impulse studio, deploy your model as a **Particle Library**

1. From this folder remove the folders `edge-impulse-sdk`, `tflite-model` and `model-parameters`

1. Copy the equally naming folders from the downloaded model into the `src` folder

1. Rebuild and flash the firmware to device

## Particle Workbench
To build the firmware and upload a binary to the Photon 2, Particle provides the Workbench VsCode extension. Please follow the [Particle Workbench](https://www.particle.io/workbench/) guide lines on setting up the tool


### Building and flashing firmware
1. In Workbench, select **Particle: Import Project** and select the `project.properties` file in the directory that you just downloaded and extracted.

1. Use **Particle: Configure Project for Device** and select **deviceOS@5.5.0** and choose a target. (e.g. **P2** , this option is also used for the Photon 2).

1. Compile with  **Particle: Compile application (local)**

1. Flash with **Particle: Flash application (local)**


> At this time you cannot use the **Particle: Cloud Compile** or **Particle: Cloud Flash** options; local compilation is required.


## Building firmware using Docker

Next to the Workbench extension, this folder contains a Dockerfile that allows building of Particle firmware.

### Create Docker image and run container
Build the Docker container:
```
docker build -t particle-build .
```

Run the container and build the firmware
```
docker run --rm -it -v $PWD:/app particle-build /bin/bash -c "./particle-build.sh --build"
```

### Flash binary to device

Flashing your Particle device requires the Particle command line tool. Follow these [instructions](https://docs.particle.io/getting-started/developer-tools/cli/) to install the tools.

Once the tool is installed, following command is used to upload a binary to the device:
```
particle flash --local target/p2/app.bin
```
