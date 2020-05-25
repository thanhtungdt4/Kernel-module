device tree node:

                example_node@1 {
                        compatible = "example-node";
                        reg = <0xd000 0x100>;
                        pwms = <&pm6150l_lpg 0 1000000>;
                        pwm-names = "pwm-led";
                        my_name = "Example";
                        author = "author";
                        my_version = <1>;
                        array = <1 2 3 4 5>;
                        led = <1>;
                };
