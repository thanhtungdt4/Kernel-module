Add below lines to node "uart0: serial@7e201000" in bcm283x.dtsi
                        hc06-bluetooth {
                                compatible = "CSR,hc06-bluetooth";
                                current-speed = <9600>;
                                pinctrl-0 = <&uart0_gpio14>;
                                pinctrl-names = "default";
                        };
uart slave node

