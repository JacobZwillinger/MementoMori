// Minimal test sketch for reTerminal E1001
// Just tests serial output and basic functionality

void setup() {
  Serial0.begin(115200);  // UART0 - outputs to /dev/cu.usbserial-110
  delay(500);

  Serial0.println("\n\n=== MEMENTO MORI TEST ===");
  Serial0.println("Serial output working!");
  Serial0.println("Hardware: reTerminal E1001");
  Serial0.println("Test completed successfully");

  // Don't go to sleep, just loop
}

void loop() {
  delay(5000);
  Serial0.println("Still running...");
}
