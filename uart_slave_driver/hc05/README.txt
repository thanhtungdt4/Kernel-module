Add below lines to node "uart0: serial@7e201000" in bcm283x.dtsi
                        hc06-bluetooth {
                                compatible = "csr,hc05-bluetooth";
                                current-speed = <9600>;
                                status = "okay";
                        };

