# แอปหลายหน้า (Multi-Page App)

ตัวอย่างนี้สร้างแอปพลิเคชัน 3 หน้าสมบูรณ์ สาธิต workflow ของ page manager ทั้งหมด: หน้า Home พร้อมการ์ดนำทาง 2 ใบ หน้า Settings พร้อม slider และหน้า Info แสดงสถิติ runtime การนำทางไหลไปข้างหน้าและย้อนกลับผ่านทั้ง 3 หน้า

หน้า Home เป็นจุดเข้าพร้อมการ์ด 2 ใบ: "Settings" และ "Info" การแตะการ์ดใดก็ตามนำทางไปยังหน้าที่เกี่ยวข้อง ทั้งสองหน้าย่อยมีปุ่ม "Back" ที่กลับไปยัง Home สาธิต navigation stack

หน้า Settings มี brightness slider ที่คงค่าไว้ใน static variable ข้ามรอบ page destroy/create สาธิตการคงข้อมูลโดยไม่ต้องใช้ global storage: สถานะหน้าอยู่รอดจากการนำทางแต่รีเซ็ตเมื่อ firmware รีบูต

หน้า Info แสดง FreeRTOS uptime เป็นวินาที free heap bytes และจำนวน navigation event ค่าเหล่านี้อัปเดตผ่าน render callback สาธิตรูปแบบ render-per-frame ที่ใช้โดยหน้า dashboard จริง

รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
