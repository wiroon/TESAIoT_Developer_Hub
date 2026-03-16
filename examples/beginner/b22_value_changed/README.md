# การซิงค์ค่าที่เปลี่ยน (Value Changed Sync)

ตัวอย่างนี้ซิงโครไนซ์ widget สองตัวด้วย `LV_EVENT_VALUE_CHANGED` event โดย Slider และ Arc gauge เชื่อมต่อกันเพื่อให้การเปลี่ยนตัวหนึ่งอัปเดตอีกตัวโดยอัตโนมัติ label แสดงค่าที่ใช้ร่วมกันปัจจุบัน

เมื่อค่า Slider เปลี่ยน callback จะอ่านค่าใหม่และนำไปใช้กับ Arc ในทำนองเดียวกันเมื่อลาก Arc callback จะอัปเดต Slider guard flag ป้องกันการเรียกซ้ำแบบไม่สิ้นสุดระหว่าง callback ทั้งสอง

รูปแบบการซิงโครไนซ์แบบสองทิศทางนี้มีประโยชน์เมื่อต้องการให้หลายวิธีในการปรับพารามิเตอร์เดียวกัน เช่น Slider หยาบและปุ่มปรับละเอียดที่ควบคุม PWM duty cycle เดียวกัน

ตัวอย่างนี้สาธิตวิธีส่ง widget reference หลายตัวผ่าน context structure รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
