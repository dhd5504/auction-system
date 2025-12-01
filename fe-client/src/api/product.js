// Backwards compatibility shim: re-export from products.js
export {
  getMyProducts as getProducts,
  createProduct,
  getAllProducts,
  getProduct,
  updateProduct,
  deleteProduct,
} from './products';
