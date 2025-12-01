// Backwards compatibility shim: re-export from rooms.js
export {
  getMyRooms as getRooms,
  createRoom,
  getAllRooms,
  getRoom,
  getMyRoom,
  deleteRoom,
} from './rooms';
