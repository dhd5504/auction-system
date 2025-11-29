const PRODUCT_BASE = 'http://localhost:8080/api/products';

async function handleResponse(response) {
  if (!response.ok) {
    const text = await response.text();
    throw new Error(text || `Request failed with status ${response.status}`);
  }
  const contentType = response.headers.get('content-type') || '';
  if (contentType.includes('application/json')) {
    return response.json();
  }
  const text = await response.text();
  return text || null;
}

export async function createProduct(product) {
  const response = await fetch(PRODUCT_BASE, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(product),
  });
  return handleResponse(response);
}

export async function getProducts() {
  const response = await fetch(PRODUCT_BASE);
  return handleResponse(response);
}
