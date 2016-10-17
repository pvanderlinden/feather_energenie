/* RFM69 library and code by Felix Rusu - felix@lowpowerlab.com
// Get libraries at: https://github.com/LowPowerLab/
// Make sure you adjust the settings in the configuration section below !!!
// **********************************************************************************
// Copyright Felix Rusu, LowPowerLab.com
// Library and code by Felix Rusu - felix@lowpowerlab.com
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE. See the GNU General Public        
// License for more details.                              
//                                                        
// You should have received a copy of the GNU General    
// Public License along with this program.
// If not, see <http://www.gnu.org/licenses></http:>.
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************/
 
/* for Feather M0  */
#define RFM69_CS      8
#define RFM69_IRQ     3
#define RFM69_RST     4

#define LED           13  // onboard blinky

void setup() {
  serial_init();
  
  led_init(LED);
  
  Serial.println("Feather RFM69HCW radio initialization");

  Serial.println("Init radio SPI");
  radio_init_spi(RFM69_CS);
  
  Serial.println("Reset radio");
  radio_reset(RFM69_RST);

  Serial.print("RF69 version: 0x");
  Serial.println(radio_version(), HEX);

  Serial.println("Init radio energenie");
  radio_init_energenie_fsk();
  if (radio_wait_ready()) {
    Serial.println("Radio ready");
  } else {
    Serial.println("Radio init failed");
  }
  radio_init_interrupt(RFM69_IRQ);

  Serial.println("Radio initialized");
  blink(40, 5);

}
 
void loop() {
}









