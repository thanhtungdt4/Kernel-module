Add two nodes below to bcm283x.dtsi file

                motor1: pwm-motor1 {
                        compatible = "pwm-motor1-control";
                        pwms = <&pwm 0 6000000>;
                        pwm-names = "motor1";
                        pinctrl-0 = <&pwm0_gpio12>;
                        pinctrl-names = "default";
                        dev_id = <1>;
                };

                motor2: pwm-motor2 {
                        compatible = "pwm-motor2-control";
                        pwms = <&pwm 1 6000000>;
                        pwm-names = "motor2";
                        pinctrl-0 = <&pwm1_gpio19>;
                        pinctrl-names = "default";
                        dev_id = <2>;
                };

