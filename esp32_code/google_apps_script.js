/**
 * ============================================================
 *  Google Apps Script — RFID Attendance to Google Sheets
 *  Sheet URL: https://docs.google.com/spreadsheets/d/1P2ha8nDAZhJvj-msMAYSVoVtkTgzFKA_4Rkki2IkNl8/edit
 *
 *  SETUP STEPS:
 *  1. Open your Google Sheet
 *  2. Extensions → Apps Script
 *  3. Paste this entire code (replace existing)
 *  4. Save (Ctrl+S)
 *  5. Deploy → New Deployment
 *     • Type: Web app
 *     • Execute as: Me
 *     • Who has access: Anyone
 *  6. Click Deploy → Authorize → Copy the Web App URL
 *  7. Paste URL into ESP32 code as GOOGLE_SHEET_URL
 * ============================================================
 */

// ─── CONFIGURATION ───────────────────────────────────────────
const SHEET_NAME  = "Attendance";   // Tab name in your spreadsheet
const HEADER_ROW  = ["#", "Name", "Student ID", "UID", "Department", "Date", "Time", "Status", "Timestamp"];
const MAX_ROWS    = 10000;          // Safety limit

// ─── GET REQUEST HANDLER (called by ESP32) ───────────────────
function doGet(e) {
  try {
    const params = e.parameter;

    // Validate required fields
    if (!params.name || !params.uid) {
      return jsonResponse({ success: false, error: "Missing required fields: name, uid" });
    }

    // Extract parameters (with defaults)
    const name      = params.name       || "Unknown";
    const studentId = params.studentId  || "N/A";
    const uid       = params.uid        || "N/A";
    const department = params.department || "General";
    const date      = params.date       || getTodayDate();
    const time      = params.time       || getCurrentTime();
    const status    = params.status     || "Present";

    // Get or create sheet
    const sheet = getOrCreateSheet();

    // Ensure header row
    ensureHeader(sheet);

    // Get next row number
    const lastRow  = sheet.getLastRow();
    const rowNum   = Math.max(0, lastRow - 1) + 1;  // Exclude header

    // Append attendance record
    sheet.appendRow([
      rowNum,
      name,
      studentId,
      uid,
      department,
      date,
      time,
      status,
      new Date().toISOString()
    ]);

    // Apply formatting to new row
    formatLastRow(sheet, lastRow + 1, status);

    Logger.log(`[OK] Recorded: ${name} | ${uid} | ${date} ${time}`);

    return jsonResponse({
      success:  true,
      message:  `Attendance recorded for ${name}`,
      row:      rowNum,
      data:     { name, studentId, uid, department, date, time, status }
    });

  } catch (err) {
    Logger.log("[ERR] " + err.message);
    return jsonResponse({ success: false, error: err.message });
  }
}

// ─── POST REQUEST HANDLER (alternative — JSON body) ──────────
function doPost(e) {
  try {
    let data;
    try {
      data = JSON.parse(e.postData.contents);
    } catch (_) {
      data = e.parameter;
    }

    const sheet = getOrCreateSheet();
    ensureHeader(sheet);

    const lastRow = sheet.getLastRow();
    const rowNum  = Math.max(0, lastRow - 1) + 1;

    sheet.appendRow([
      rowNum,
      data.name       || "Unknown",
      data.studentId  || "N/A",
      data.uid        || "N/A",
      data.department || "General",
      data.date       || getTodayDate(),
      data.time       || getCurrentTime(),
      data.status     || "Present",
      new Date().toISOString()
    ]);

    formatLastRow(sheet, lastRow + 1, data.status || "Present");

    return jsonResponse({ success: true, message: "Recorded via POST" });

  } catch (err) {
    return jsonResponse({ success: false, error: err.message });
  }
}

// ─── HELPER: Get or create the Attendance sheet ──────────────
function getOrCreateSheet() {
  const ss    = SpreadsheetApp.getActiveSpreadsheet();
  let   sheet = ss.getSheetByName(SHEET_NAME);

  if (!sheet) {
    sheet = ss.insertSheet(SHEET_NAME);
    // Set column widths
    sheet.setColumnWidth(1, 40);   // #
    sheet.setColumnWidth(2, 160);  // Name
    sheet.setColumnWidth(3, 120);  // Student ID
    sheet.setColumnWidth(4, 100);  // UID
    sheet.setColumnWidth(5, 120);  // Department
    sheet.setColumnWidth(6, 100);  // Date
    sheet.setColumnWidth(7, 100);  // Time
    sheet.setColumnWidth(8, 90);   // Status
    sheet.setColumnWidth(9, 200);  // Timestamp
  }

  return sheet;
}

// ─── HELPER: Ensure header row exists ────────────────────────
function ensureHeader(sheet) {
  if (sheet.getLastRow() === 0) {
    const headerRange = sheet.getRange(1, 1, 1, HEADER_ROW.length);
    headerRange.setValues([HEADER_ROW]);
    headerRange.setFontWeight("bold");
    headerRange.setBackground("#1a1a2e");
    headerRange.setFontColor("#f5a623");
    headerRange.setFontSize(11);
    sheet.setFrozenRows(1);
  }
}

// ─── HELPER: Format the newly added row ──────────────────────
function formatLastRow(sheet, rowIndex, status) {
  const range = sheet.getRange(rowIndex, 1, 1, HEADER_ROW.length);

  // Alternating row colors
  if (rowIndex % 2 === 0) {
    range.setBackground("#f8f9fa");
  }

  // Color status cell
  const statusCell = sheet.getRange(rowIndex, 8);  // Column H = Status
  if (status === "Present") {
    statusCell.setBackground("#d4edda");
    statusCell.setFontColor("#155724");
  } else {
    statusCell.setBackground("#f8d7da");
    statusCell.setFontColor("#721c24");
  }
  statusCell.setFontWeight("bold");
}

// ─── HELPER: Get today's date ─────────────────────────────────
function getTodayDate() {
  const d = new Date();
  return Utilities.formatDate(d, Session.getScriptTimeZone(), "yyyy-MM-dd");
}

// ─── HELPER: Get current time ─────────────────────────────────
function getCurrentTime() {
  const d = new Date();
  return Utilities.formatDate(d, Session.getScriptTimeZone(), "hh:mm aa");
}

// ─── HELPER: JSON response ────────────────────────────────────
function jsonResponse(obj) {
  return ContentService
    .createTextOutput(JSON.stringify(obj))
    .setMimeType(ContentService.MimeType.JSON);
}

// ─── UTILITY: Clear all attendance data ──────────────────────
function clearAttendance() {
  const sheet = getOrCreateSheet();
  if (sheet.getLastRow() > 1) {
    sheet.deleteRows(2, sheet.getLastRow() - 1);
  }
}

// ─── UTILITY: Generate summary report ────────────────────────
function generateSummary() {
  const sheet  = getOrCreateSheet();
  const data   = sheet.getDataRange().getValues();

  if (data.length <= 1) {
    SpreadsheetApp.getUi().alert("No data to summarize.");
    return;
  }

  const rows = data.slice(1);  // Skip header
  const byDate = {};

  rows.forEach(row => {
    const date   = row[5];
    const status = row[7];
    if (!byDate[date]) byDate[date] = { present: 0, absent: 0 };
    if (status === "Present") byDate[date].present++;
    else byDate[date].absent++;
  });

  let report = "=== Attendance Summary ===\n";
  Object.keys(byDate).sort().forEach(date => {
    const { present, absent } = byDate[date];
    const total = present + absent;
    const rate  = total > 0 ? Math.round((present / total) * 100) : 0;
    report += `${date}: ${present} present, ${absent} absent (${rate}% attendance)\n`;
  });

  SpreadsheetApp.getUi().alert(report);
}

// ─── MENU (adds to Google Sheets UI) ─────────────────────────
function onOpen() {
  SpreadsheetApp.getUi()
    .createMenu("📡 RFID Attendance")
    .addItem("Generate Summary", "generateSummary")
    .addSeparator()
    .addItem("Clear All Records", "clearAttendance")
    .addToUi();
}
