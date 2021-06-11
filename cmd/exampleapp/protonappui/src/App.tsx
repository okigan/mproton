import React from 'react';
import logo from './logo.svg';
import './App.css';

interface pingInterface {
  (message: string): Promise<string>;
}

declare var ping: pingInterface;

interface Window {
  webkit?: any;
  chrome?: any;
  external?: any;
  proton?: any;
}

declare var window: Window;

function App() {
  const submitForm = async (event: React.FormEvent<HTMLFormElement>) => {
    event.preventDefault();
    try {
      var promise = window.proton.mycallback1.invoke(
        "Hello from typescript",
        "and warm hugs too",
        { "Name": "Batman", "Age": 42 }
      ).then(
        function (result: any) {
          console.log(result);
        },
        function (err: any) {
          console.log(err);
        })
        ;
    } catch (err) {
      console.log('got error: ' + err);
    }
  }
  return (
    <div className="wrapper">
      <img src={logo} className="App-logo" alt="logo" />
      <h1>Teleport Cluster</h1>
      <form onSubmit={submitForm}>
        <fieldset>
          <label>
            <p>Cluster</p>
            <input name="name" placeholder="host:port" />
          </label>
        </fieldset>
        <button type="submit">Submit</button>
      </form>
    </div>
  )
}

export default App;
