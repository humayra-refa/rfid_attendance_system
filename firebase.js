import { initializeApp } from "https://www.gstatic.com/firebasejs/10.12.0/firebase-app.js";
import { getFirestore, collection, addDoc, getDocs, getDoc, setDoc, doc, query, where, onSnapshot, serverTimestamp, deleteDoc, updateDoc } from "https://www.gstatic.com/firebasejs/10.12.0/firebase-firestore.js";
import { getAuth, signInWithEmailAndPassword, signOut, onAuthStateChanged, sendPasswordResetEmail } from "https://www.gstatic.com/firebasejs/10.12.0/firebase-auth.js";

const firebaseConfig = {
  apiKey: "AIzaSyDSL2XxgPMD4eY-g6ykXvYZCsTW46jXHRQ",
  authDomain: "rfid-attendence-f28aa.firebaseapp.com",
  projectId: "rfid-attendence-f28aa",
  storageBucket: "rfid-attendence-f28aa.firebasestorage.app",
  messagingSenderId: "613381904772",
  appId: "1:613381904772:web:8ca40f62428d9e1ece25d1",
  measurementId: "G-D80JJN39GG"
};

const app  = initializeApp(firebaseConfig);
const db   = getFirestore(app);
const auth = getAuth(app);

export function loginAdmin(email, password) { return signInWithEmailAndPassword(auth, email, password); }
export function logoutAdmin() { return signOut(auth); }
export function onAuthChange(callback) { return onAuthStateChanged(auth, callback); }
export async function resetAdminPassword(email) { return sendPasswordResetEmail(auth, email); }

export async function addStudent(data) { return setDoc(doc(db, "students", data.uid), { ...data, createdAt: serverTimestamp() }); }
export async function updateStudentData(uid, updatedData) { return updateDoc(doc(db, "students", uid), updatedData); }
export async function removeStudent(uid) { return deleteDoc(doc(db, "students", uid)); }
export async function getAllStudents() { const snap = await getDocs(collection(db, "students")); return snap.docs.map(d => ({ id: d.id, ...d.data() })); }
export async function getStudentByUID(uid) { const snap = await getDoc(doc(db, "students", uid)); return snap.exists() ? { id: snap.id, ...snap.data() } : null; }

export async function recordAttendance(data) { return addDoc(collection(db, "attendence"), { ...data, timestamp: serverTimestamp() }); }
export async function hasMarkedToday(uid, date) { const q = query(collection(db, "attendence"), where("uid", "==", uid), where("date", "==", date)); const snap = await getDocs(q); return !snap.empty; }
export async function getAllAttendance() { const snap = await getDocs(collection(db, "attendence")); return snap.docs.map(d => ({ id: d.id, ...d.data() })); }
export function listenTodayAttendance(date, callback) { const q = query(collection(db, "attendence"), where("date", "==", date)); return onSnapshot(q, snap => { callback(snap.docs.map(d => ({ id: d.id, ...d.data() }))); }); }
export function listenAllAttendance(callback) { const q = query(collection(db, "attendence")); return onSnapshot(q, snap => { callback(snap.docs.map(d => ({ id: d.id, ...d.data() }))); }); }

export { db, auth };