Node for lcd
                spi: spi@7e204000 {
                        compatible = "brcm,bcm2835-spi";
                        reg = <0x7e204000 0x200>;
                        interrupts = <2 22>;
                        clocks = <&clocks BCM2835_CLOCK_VPU>;
                        #address-cells = <1>;
                        #size-cells = <0>;
                        status = "okay";

                        lcdILI9431@0 {
                                compatible = "tft,lcdILI9341";
                                reg = <0>;
                                spi-max-frequency = <20000000>;
                        };
                };

