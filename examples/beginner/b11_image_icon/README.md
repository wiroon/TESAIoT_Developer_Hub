# แสดงไอคอนรูปภาพ (Image Icon Display)

ตัวอย่างนี้แสดงไอคอน symbol ในตัวของ LVGL เรียงเป็นแถวแนวนอน LVGL symbol เช่น WiFi, Settings, Bluetooth, Battery และ Home ถูก render เป็นอักขระฟอนต์พิเศษโดยใช้ค่าคงที่ `LV_SYMBOL_*`

แต่ละไอคอนถูกสร้างเป็น Label พร้อมข้อความ symbol จัดรูปแบบด้วยฟอนต์ขนาดใหญ่และสีที่แตกต่างจาก Palette ของ LVGL ไอคอนวางใน flex row container ที่มีระยะห่างเท่ากัน สาธิตวิธีสร้าง icon toolbar และ status bar

ด้านล่างแต่ละไอคอนมี Label ข้อความเล็กระบุชื่อ symbol layout แบบสองแถวนี้ใช้ flex container ซ้อนกันเพื่อการจัดตำแหน่งที่เรียบร้อย Symbol เป็นส่วนหนึ่งของฟอนต์ในตัว LVGL ไม่ต้องใช้ไฟล์รูปภาพภายนอก

ตัวอย่างนี้เป็นข้อมูลอ้างอิงสำหรับ symbol ในตัวทั้งหมด รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
